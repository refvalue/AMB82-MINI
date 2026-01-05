import { TlvReader, TlvWriter } from "./tlv";
import { resizeArray } from "./util";

export const RequestType = Object.freeze({
    GET_SYSTEM_INFO: 0,
    SET_SYSTEM_TIME: 1,
    GET_RECORDING_SCHEDULE: 2,
    SET_RECORDING_SCHEDULE: 3,
});

export class SdCardInfo {
    constructor(freeSpaceBytes, usedSpaceBytes) {
        if (typeof freeSpaceBytes !== 'number') {
            throw new TypeError('`freeSpaceBytes` must be a number.');
        }

        if (typeof usedSpaceBytes !== 'number') {
            throw new TypeError('`usedSpaceBytes` must be a number.');
        }

        this.freeSpaceBytes = freeSpaceBytes;
        this.usedSpaceBytes = usedSpaceBytes;
    }

    totalSpaceBytes() {
        return this.freeSpaceBytes + this.usedSpaceBytes;
    }

    usedSpaceRatio() {
        return this.totalSpaceBytes === 0 ? 0 : this.freeSpaceBytes / this.totalSpaceBytes;
    }
}

export class HotspotInfo {
    constructor(ssid, password, enabled) {
        if (typeof ssid !== 'string') {
            throw new TypeError('`ssid` must be a string.');
        }

        if (typeof password !== 'string') {
            throw new TypeError('`password` must be a string.');
        }

        if (typeof enabled !== 'boolean') {
            throw new TypeError('`enabled` must be a boolean.');
        }

        this.ssid = ssid;
        this.password = password;
        this.enabled = enabled;
    }
}

export class SystemInfo {
    constructor(sdcard, timestamp, hotspot) {
        if (!(sdcard instanceof SdCardInfo)) {
            throw new TypeError('`sdcard` must be a `SdCardInfo`.');
        }

        if (typeof timestamp !== 'number') {
            throw new TypeError('`timestamp` must be a number.');
        }

        if (!(hotspot instanceof HotspotInfo)) {
            throw new TypeError('`hotspot` must be a `HotspotInfo`.');
        }

        this.sdcard = sdcard;
        this.timestamp = timestamp;
        this.hotspot = hotspot;
    }
}

export class RecordingPlan {
    constructor(startTimestamp, duration) {
        if (typeof startTimestamp !== 'number') {
            throw new TypeError('`startTimestamp` must be a number.');
        }

        if (typeof duration !== 'number') {
            throw new TypeError('`duration` must be a number.');
        }

        this.startTimestamp = startTimestamp;
        this.duration = duration;
    }
}

export class RecordingSchedule {
    constructor() {
        this.schedule = new Proxy([], {
            set(target, prop, value) {
                if (!(value instanceof RecordingPlan)) {
                    throw new TypeError('The element must be a `RecordingPlan`.');
                }

                target[prop] = value;

                return true;
            }
        });
    }

    resize(newLength) {
        resizeArray(this.schedule, newLength, new RecordingPlan());
    }
}

export class SystemTimeInfo {
    constructor(timestampOrDateTime) {
        if (typeof timestamp === 'number') {
            this.timestamp = timestamp;
        }
        else if (timestampOrDateTime instanceof Date) {
            this.timestamp = Math.floor(dateTime.getTime() / 1000);
        }
        else {
            throw new TypeError('`timestampOrDateTime` must be a number or `Date`.');
        }
    }
}

export class SystemSettings {
    async getSystemInfo() {
        const buffer = await this._send('/api/v1/getSystemInfo', RequestType.GET_SYSTEM_INFO, new Uint8Array());
        const reader = new TlvReader(buffer.buffer);
        const result = new SystemInfo(new SdCardInfo(), 0, new HotspotInfo());

        reader.registerHandler(1, 'number', (_, value) => result.sdcard.freeSpaceBytes = value);
        reader.registerHandler(2, 'number', (_, value) => result.sdcard.usedSpaceBytes = value);
        reader.registerHandler(3, 'number', (_, value) => result.timestamp = value);
        reader.readAll();

        return result;
    }

    async setSystemTime(systemTimeInfo) {
        const writer = new TlvWriter();

        writer.writeMagic();
        writer.writeUInt64(1, systemTimeInfo.timestamp);
        await this._send('/api/vi/setSystemTime', RequestType.SET_SYSTEM_TIME, writer.toArrayBuffer());
    }

    async getRecordingSchedule() {
        const buffer = await this._send('/api/v1/getRecordingSchedule', RequestType.GET_RECORDING_SCHEDULE, new Uint8Array());
        const reader = new TlvReader(buffer.buffer);
        const result = new RecordingSchedule();

        for (let i = 0; i < 8; i++) {
            reader.registerHandler(100 + i * 2, 'number', (_, timestamp) => {
                result.resize(i + 1);
                result[i].startTimestamp = timestamp;
            });

            reader.registerHandler(101 + i * 2, 'number', (_, duration) => {
                result.resize(i + 1);
                result[i].duration = duration;
            });
        }

        reader.readAll();

        return result;
    }

    async setRecordingSchedule(schedule) {
        const writer = new TlvWriter();

        writer.writeMagic();

        const length = Math.min(schedule.schedule.length, 8);

        for (let i = 0; i < length; i++) {
            const item = schedule.schedule[i];

            writer.writeUInt64(100 + i * 2, item.startTimestamp);
            writer.writeUInt32(101 + i * 2, item.duration);
        }

        await this._send('/api/v1/setRecordingSchedule', RequestType.SET_RECORDING_SCHEDULE, writer.toArrayBuffer());
    }

    async _send(url, type, data) {
        const buffer = new Uint8Array(data.length + 1);

        buffer[0] = type;
        buffer.set(data, 1);

        const response = await fetch(url, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/octet-stream'
            },
            body: data,
        });

        if (!response.ok) {
            throw new Error(`Error ${response.status} (${response.statusText}): ${await response.text()}`);
        }

        return response.bytes();
    }
}
