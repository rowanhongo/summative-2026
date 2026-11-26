# Project 3: Python C Extension for Sensor Data

sensor_processor is a Python module written in C with the Python C API. It filters sensor readings and calculates statistics. All the number work is done in C.

## Files

- sensor_processor.c: the extension. It has the five functions, the method table, the module definition and the init function.
- setup.py: the setuptools build file.
- test_sensor.py: the test program.
- test_output.txt: the saved output of test_sensor.py.
- benchmark.py: compares the C functions with plain Python.
- benchmark_results.txt: the saved output of benchmark.py.
- build_output.txt: the saved output of the build.

## Environment

Ubuntu 24.04 on WSL2, GCC 13.3.0, Python 3.12.3 and setuptools 68.1.2. The Python headers are needed (the python3-dev package).

## Build

```
python3 setup.py build_ext --inplace
```

This makes sensor_processor.cpython-312-x86_64-linux-gnu.so in the same folder.

## Run

```
python3 test_sensor.py
python3 benchmark.py
```

## The functions

All of them take a list or a tuple of numbers. Ints are accepted and treated as floats.

| Function | Returns | Example |
| --- | --- | --- |
| filter_readings(data, minimum, maximum) | a new list with the readings from minimum to maximum, both included | filter_readings([10.5, 25.0, 110.0, 35.5], 20.0, 40.0) gives [25.0, 35.5] |
| count_outliers(data, minimum, maximum) | the number of readings outside the range | count_outliers([10.5, 25.0, 110.0, 35.5], 20.0, 40.0) gives 2 |
| moving_average(data, window) | a list of the average of every window | moving_average([10.0, 20.0, 30.0, 40.0], 2) gives [15.0, 25.0, 35.0] |
| maximum_difference(data) | the maximum minus the minimum | maximum_difference([10.0, 25.0, 40.0, 15.0]) gives 30.0 |
| sensor_summary(data) | a dictionary with samples, minimum, maximum, average and above_average | sensor_summary([10.0, 20.0, 30.0, 40.0]) gives {'samples': 4, 'minimum': 10.0, 'maximum': 40.0, 'average': 25.0, 'above_average': 2} |

## Errors

| Problem | Exception and message |
| --- | --- |
| data is not a list or a tuple (for example a string, a number or a set) | TypeError: expected a list or tuple |
| data is empty | ValueError: empty dataset |
| an item is not a number | TypeError: must be real number, not str |
| minimum is greater than maximum | ValueError: minimum must not be greater than maximum |
| window is smaller than 1 or bigger than the data | ValueError: window must be between 1 and the number of readings |
| window is not an integer | TypeError: 'float' object cannot be interpreted as an integer |

An empty dataset raises an error in all five functions, also in filter_readings and count_outliers.

## Expected output

test_output.txt has the full output. The start of it:

```
--- normal use ---
filter_readings(data, 20.0, 40.0) -> [25.0, 35.5]
count_outliers(data, 20.0, 40.0) -> 2
moving_average([10, 20, 30, 40], 2) -> [15.0, 25.0, 35.0]
maximum_difference([10, 25, 40, 15]) -> 30.0
sensor_summary([10, 20, 30, 40]) -> {'samples': 4, 'minimum': 10.0, 'maximum': 40.0, 'average': 25.0, 'above_average': 2}
```

## Time complexity

n is the number of readings and w is the window size.

| Function | Time | Extra memory |
| --- | --- | --- |
| filter_readings | O(n) | O(n) |
| count_outliers | O(n) | O(n) |
| moving_average | O(n) | O(n) |
| maximum_difference | O(n) | O(n) |
| sensor_summary | O(n), two passes | O(n) |

The extra memory is the C array that holds the copy of the data, plus the result list. moving_average does not add up w numbers for every window. It adds the new reading and removes the old one, so it stays O(n) and not O(n * w).

## Performance

benchmark.py uses 1,000,000 random readings and keeps the best of 3 runs. Times are in seconds.

```
function                 python          C  speedup
filter_readings         0.2782s    0.1772s     1.6x
count_outliers          0.2369s    0.0478s     5.0x
moving_average          1.0100s    0.2745s     3.7x
maximum_difference      0.1757s    0.0339s     5.2x
sensor_summary          0.3778s    0.0479s     7.9x
```

The numbers change a little between runs. I think filter_readings gains the least because most of its time goes into building the new list of Python floats. I did not measure that on its own. maximum_difference is compared against max() and min(), which are already written in C.

## Memory

- toArray() uses one malloc() per call, and every path frees it, also on errors.
- PySequence_Fast_GET_ITEM gives a borrowed reference, so nothing is released for the items.
- PyList_SET_ITEM takes over the reference to the new float, so the list owns it.
- If creating a float fails, the list is released with Py_DECREF before returning NULL.
- Py_BuildValue creates the objects for the dictionary, and the dictionary owns them.
