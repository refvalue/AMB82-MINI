export const TlvConstants = {
    Magic: new Uint8Array(['A'.charCodeAt(0), 'M'.charCodeAt(0), 'B'.charCodeAt(0), '8'.charCodeAt(0), '2'.charCodeAt(0), 'V'.charCodeAt(0), 'B'.charCodeAt(0), 'X'.charCodeAt(0), 0x01]),
    TypeLengthSize: 2
};

export class TlvReader {
    constructor(arrayBuffer) {
        if (!(arrayBuffer instanceof ArrayBuffer)) {
            throw new TypeError('Expected ArrayBuffer.');
        }

        this._buffer = new Uint8Array(arrayBuffer);
        this._offset = 0;
        this._handlers = new Map();
        this._ensureMagic();
    }

    registerHandler(type, dataType, handler) {
        if (typeof handler !== 'function') {
            throw new TypeError('`handler` must be a function.');
        }

        if (!['number', 'string'].includes(dataType)) {
            throw new TypeError('`dataType` must be "number" or "string".');
        }

        this._handlers.set(type, { dataType, handler });
    }

    _ensureMagic() {
        const magic = TlvConstants.Magic;

        for (let i = 0; i < magic.length; i++) {
            if (this._buffer[i] !== magic[i]) {
                throw new Error('Invalid TLV magic number.');
            }
        }

        this._offset = magic.length;
    }

    readAll() {
        const buffer = this._buffer;
        let offset = this._offset;

        while (offset + TlvConstants.TypeLengthSize <= buffer.length) {
            const type = buffer[offset];
            const length = buffer[offset + 1];

            if (offset + TlvConstants.TypeLengthSize + length > buffer.length) {
                break;
            }

            const valueBytes = buffer.slice(offset + TlvConstants.TypeLengthSize, offset + TlvConstants.TypeLengthSize + length);
            const entry = this._handlers.get(type);

            if (entry) {
                let value;

                if (entry.dataType === 'number') {
                    switch (valueBytes.length) {
                        case 1: value = valueBytes[0]; break;
                        case 2: value = (valueBytes[0] << 8) | valueBytes[1]; break;
                        case 4: value = (valueBytes[0] * 2 ** 24) + (valueBytes[1] << 16) + (valueBytes[2] << 8) + valueBytes[3]; break;
                        case 8:
                            value = 0n;
                            for (let i = 0; i < 8; i++) {
                                value = (value << 8n) | BigInt(valueBytes[i]);
                            }
                            break;
                        default: throw new Error(`Unsupported number size ${valueBytes.length} for type ${type}.`);
                    }
                } else if (entry.dataType === 'string') {
                    value = new TextDecoder().decode(valueBytes).replace(/\0$/, '');
                }

                entry.handler(type, value);
            }

            offset += TlvConstants.TypeLengthSize + length;
        }
    }
}

export class TlvWriter {
    constructor() {
        this._chunks = [];
    }

    writeMagic() {
        this._chunks.push(TlvConstants.Magic);
    }

    write(type, value) {
        if (!(value instanceof Uint8Array)) {
            throw new TypeError('`value` must be Uint8Array.');
        }

        if (value.length > 255) {
            throw new RangeError('`value` too long.');
        }

        this._chunks.push(new Uint8Array([type, value.length]));
        this._chunks.push(value);
    }

    writeUInt8(type, value) {
        this.write(type, new Uint8Array([value]));
    }

    writeUInt16(type, value) {
        const buffer = new Uint8Array(2);

        buffer[0] = (value >> 8) & 0xFF;
        buffer[1] = value & 0xFF;
        this.write(type, buffer);
    }

    writeUInt32(type, value) {
        const buffer = new Uint8Array(4);

        buffer[0] = (value >> 24) & 0xFF;
        buffer[1] = (value >> 16) & 0xFF;
        buffer[2] = (value >> 8) & 0xFF;
        buffer[3] = value & 0xFF;
        this.write(type, buffer);
    }

    writeUInt64(type, value) {
        let bigValue = typeof value === 'bigint' ? value : BigInt(value);
        const buffer = new Uint8Array(8);

        for (let i = 7; i >= 0; i--) {
            buffer[i] = Number(bigValue & 0xFFn);
            bigValue >>= 8n;
        }

        this.write(type, buffer);
    }

    writeString(type, value, maxLength) {
        const encoder = new TextEncoder();
        let bytes = encoder.encode(value);

        if (bytes.length > maxLength) {
            bytes = bytes.slice(0, maxLength);
        }

        // Add null terminator
        const buffer = new Uint8Array(bytes.length + 1);
        buffer.set(bytes);
        buffer[bytes.length] = 0;
        this.write(type, buffer);
    }

    toArrayBuffer() {
        const totalLength = this._chunks.reduce((sum, c) => sum + c.length, 0);
        const result = new Uint8Array(totalLength);
        let offset = 0;

        for (const chunk of this._chunks) {
            result.set(chunk, offset);
            offset += chunk.length;
        }

        return result.buffer;
    }
}
