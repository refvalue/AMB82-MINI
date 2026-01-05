export function resizeArray(arr, newLength, initValue = undefined) {
    if (!Array.isArray(arr)) {
        throw new TypeError('Expected an array.');
    }

    const length = arr.length;

    if (newLength < length) {
        arr.length = newLength;
    } else if (newLength > length) {
        for (let i = length; i < newLength; i++) {
            arr[i] = initValue;
        }
    }

    return arr;
}
