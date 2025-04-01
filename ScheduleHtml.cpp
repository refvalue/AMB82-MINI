#include "Resources.hpp"

constexpr char scheduleHtml[] = R"AAAAA(<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Recording Schedule Manager</title>
    <link rel="stylesheet" href="styles.css">
</head>

<body>
    <div id="loading" class="loading-overlay">
        <div class="spinner"></div>
    </div>

    <div class="container">
        <button onclick="window.location.href='/system-info.html'">System Info</button>
        <h1>Recording Schedule Manager</h1>

        <!-- Date and Time Selection -->
        <div class="form-group">
            <label for="date">Select Date</label>
            <input type="date" id="date" required>
        </div>
        <div class="form-group">
            <label for="start-time">Start Time (15-minute intervals)</label>
            <select id="start-time" required>
                <!-- 15-minute intervals -->
            </select>
        </div>
        <div class="form-group">
            <label for="end-time">End Time (15-minute intervals)</label>
            <select id="end-time" required>
                <!-- 15-minute intervals -->
            </select>
        </div>

        <button onclick="addRecordingPlan()">Add Plan</button>
        <button onclick="fetchRecordingSchedule()">Refresh List</button>

        <div class="schedule-list">
            <h2>Current Recording Plans</h2>
            <table id="schedule-table">
                <thead>
                    <tr>
                        <th>Date</th>
                        <th>Start Time</th>
                        <th>End Time</th>
                        <th>Duration</th>
                        <th>Action</th>
                    </tr>
                </thead>
                <tbody>
                    <!-- Recording schedule entries will be dynamically inserted here -->
                </tbody>
            </table>
        </div>
    </div>

    <script>
        let recordingSchedule = [];

        function getDurationInHours(time1, time2) {
            const [h1, m1] = time1.split(":").map(Number);
            const [h2, m2] = time2.split(":").map(Number);

            const date1 = new Date(0, 0, 0, h1, m1);
            const date2 = new Date(0, 0, 0, h2, m2);

            return (date2 - date1) / (1000 * 60 * 60);
        }

        function makeDateString(date) {
            const yyyy = date.getFullYear();
            const mm = String(date.getMonth() + 1).padStart(2, '0');
            const dd = String(date.getDate()).padStart(2, '0');

            return `${yyyy}-${mm}-${dd}`;
        }

        function makeHourMinuteString(date) {
            const hh = String(date.getHours()).padStart(2, '0');
            const mm = String(date.getMinutes()).padStart(2, '0');

            return `${hh}:${mm}`;
        }

        function setDateToday() {
            document.getElementById('date').value = makeDateString(new Date());
        }

        function generateTimeOptions() {
            const timeOptions = [];

            for (let hour = 0; hour < 24; hour++) {
                for (let minute = 0; minute < 60; minute += 15) {
                    const hourStr = hour < 10 ? '0' + hour : hour;
                    const minuteStr = minute < 10 ? '0' + minute : minute;
                    timeOptions.push(`${hourStr}:${minuteStr}`);
                }
            }

            const startTimeSelect = document.getElementById('start-time');
            const endTimeSelect = document.getElementById('end-time');

            timeOptions.forEach(time => {
                const startOption = document.createElement('option');
                startOption.value = time;
                startOption.textContent = time;
                startTimeSelect.appendChild(startOption);

                const endOption = document.createElement('option');
                endOption.value = time;
                endOption.textContent = time;
                endTimeSelect.appendChild(endOption);
            });
        }

        function renderRecordingSchedule() {
            const scheduleTableBody = document.getElementById('schedule-table').getElementsByTagName('tbody')[0];

            // Clears the current rows.
            scheduleTableBody.innerHTML = '';

            recordingSchedule.forEach((schedule, index) => {
                const row = scheduleTableBody.insertRow();

                row.insertCell(0).textContent = schedule.date;
                row.insertCell(1).textContent = schedule.start;
                row.insertCell(2).textContent = schedule.end;
                row.insertCell(3).textContent = `${schedule.duration} h`;

                const deleteCell = row.insertCell(4);
                const deleteButton = document.createElement('button');

                deleteButton.textContent = 'Delete';
                deleteButton.addEventListener('click', async () => await deleteRecordingPlan(index));
                deleteCell.appendChild(deleteButton);
            });
        }

        async function addRecordingPlan() {
            const selectedDate = document.getElementById('date').value;
            const startTime = document.getElementById('start-time').value;
            const endTime = document.getElementById('end-time').value;

            if (!selectedDate || !startTime || !endTime) {
                alert("Please select a date and valid start/end times.");
                return;
            }

            if (startTime >= endTime) {
                alert("Start time must be earlier than end time.");
                return;
            }

            // Checks for conflicts with existing schedules
            let conflictFound = false;

            for (const item of recordingSchedule) {
                if (item.date === selectedDate) {
                    const scheduleStart = item.start;
                    const scheduleEnd = item.end;

                    if ((startTime >= scheduleStart && startTime < scheduleEnd) ||
                        (endTime > scheduleStart && endTime <= scheduleEnd) ||
                        (startTime <= scheduleStart && endTime >= scheduleEnd)) {
                        conflictFound = true;
                        break;
                    }
                }
            }

            if (conflictFound) {
                alert("Conflict detected! Please choose a different time.");
                return;
            }

            const duration = getDurationInHours(startTime, endTime);

            recordingSchedule.push({
                date: selectedDate,
                start: startTime,
                end: endTime,
                duration: duration,
                origin: {
                    startTimestamp: Math.floor((new Date(`${selectedDate}T${startTime}`).getTime() / 1000 - new Date().getTimezoneOffset() * 60)),
                    duration: duration * 3600,
                },
            });

            if (await updateRecordingSchedule('Recording plan successfully added.', 'Error adding the recording plan') === false) {
                recordingSchedule.pop();
            }

            renderRecordingSchedule();
        }

        async function deleteRecordingPlan(index) {
            if (confirm("Are you sure you want to delete this schedule?")) {
                const item = recordingSchedule.splice(index, 1)[0];

                if (await updateRecordingSchedule('Recording plan successfully deleted.', 'Error deleting the recording plan') === false) {
                    recordingSchedule.splice(index, 0, item);
                }

                renderRecordingSchedule();
            }
        }

        async function fetchRecordingSchedule() {
            showLoading();

            try {
                const response = await fetch('/api/v1/currentSchedule');

                if (!response.ok) {
                    throw new Error(response.statusText);
                }

                const data = await response.json();

                if (data.success !== true) {
                    throw new Error(data.message);
                }

                recordingSchedule.length = 0;

                for (const item of data.data.schedule) {
                    const startTime = new Date((item.startTimestamp + new Date().getTimezoneOffset() * 60) * 1000);
                    const endTime = new Date(startTime.getTime() + item.duration * 1000);

                    recordingSchedule.push({
                        date: makeDateString(startTime),
                        start: makeHourMinuteString(startTime),
                        end: makeHourMinuteString(endTime),
                        duration: item.duration / 3600,
                        origin: item,
                    });
                }

                renderRecordingSchedule();
            } catch (e) {
                alert(`Error fetching the recording schedule: ${e}`);
            }
            finally {
                hideLoading();
            }
        }

        async function updateRecordingSchedule(successMessage, failurePrefix) {
            showLoading();

            try {
                const schedule = [];

                for (const item of recordingSchedule) {
                    schedule.push(item.origin);
                }

                const response = await fetch('/api/v1/updateSchedule', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({ schedule: schedule }),
                });

                if (!response.ok) {
                    throw new Error(response.statusText);
                }

                const data = await response.json();

                if (data.success !== true) {
                    throw new Error(data.message);
                }

                alert(successMessage);

                return true;
            } catch (e) {
                alert(`${failurePrefix}: ${e}`);

                return false;
            }
            finally {
                hideLoading();
            }
        }

        function showLoading() {
            document.getElementById('loading').style.display = 'flex';
        }

        function hideLoading() {
            document.getElementById('loading').style.display = 'none';
        }

        setDateToday();
        generateTimeOptions();
        renderRecordingSchedule();
        fetchRecordingSchedule();
    </script>
</body>

</html>)AAAAA";
