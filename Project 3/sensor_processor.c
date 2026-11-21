#include <Python.h>
#include <stdlib.h>

// copies a list or tuple of numbers into a new C array of doubles
// PyFloat_AsDouble also accepts ints, so [1, 2.5] works
// the caller must free the array, one malloc per call
double *toArray(PyObject *seq, Py_ssize_t *size)
{
    double *arr;
    Py_ssize_t i;

    if (!PyList_Check(seq) && !PyTuple_Check(seq))
    {
        PyErr_SetString(PyExc_TypeError, "expected a list or tuple");
        return NULL;
    }

    *size = PySequence_Fast_GET_SIZE(seq);

    if (*size == 0)
    {
        PyErr_SetString(PyExc_ValueError, "empty dataset");
        return NULL;
    }

    arr = malloc(*size * sizeof(double));

    if (arr == NULL)
    {
        PyErr_NoMemory();
        return NULL;
    }

    for (i = 0; i < *size; i++)
    {
        // GET_ITEM gives a borrowed reference so there is nothing to DECREF
        arr[i] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(seq, i));

        // -1.0 with an error set means the item was not a number
        if (arr[i] == -1.0 && PyErr_Occurred())
        {
            free(arr);
            return NULL;
        }
    }

    return arr;
}

// O(n) time, O(n) extra space for the copy and the result
// keeps x when minimum <= x <= maximum
PyObject *filterReadings(PyObject *self, PyObject *args)
{
    PyObject *seq, *list, *item;
    double *arr;
    double minimum, maximum;
    Py_ssize_t n, i, count = 0;

    if (!PyArg_ParseTuple(args, "Odd", &seq, &minimum, &maximum))
        return NULL;

    // written this way so a nan bound is rejected too
    if (!(minimum <= maximum))
    {
        PyErr_SetString(PyExc_ValueError, "minimum must not be greater than maximum");
        return NULL;
    }

    arr = toArray(seq, &n);

    if (arr == NULL)
        return NULL;

    // keep the good readings at the front of our own copy
    for (i = 0; i < n; i++)
    {
        if (arr[i] >= minimum && arr[i] <= maximum)
        {
            arr[count] = arr[i];
            count++;
        }
    }

    list = PyList_New(count);

    if (list == NULL)
    {
        free(arr);
        return NULL;
    }

    for (i = 0; i < count; i++)
    {
        item = PyFloat_FromDouble(arr[i]);

        if (item == NULL)
        {
            Py_DECREF(list);
            free(arr);
            return NULL;
        }

        // SET_ITEM steals the reference to item
        PyList_SET_ITEM(list, i, item);
    }

    free(arr);

    return list;
}

// O(n) time, O(n) extra space for the copy
// outlier means not (minimum <= x <= maximum)
PyObject *countOutliers(PyObject *self, PyObject *args)
{
    PyObject *seq;
    double *arr;
    double minimum, maximum;
    Py_ssize_t n, i, count = 0;

    if (!PyArg_ParseTuple(args, "Odd", &seq, &minimum, &maximum))
        return NULL;

    if (!(minimum <= maximum))
    {
        PyErr_SetString(PyExc_ValueError, "minimum must not be greater than maximum");
        return NULL;
    }

    arr = toArray(seq, &n);

    if (arr == NULL)
        return NULL;

    for (i = 0; i < n; i++)
    {
        if (!(arr[i] >= minimum && arr[i] <= maximum))
            count++;
    }

    free(arr);

    return PyLong_FromSsize_t(count);
}

// O(n) time, O(n) extra space
// average of window w starting at i = (x[i] + ... + x[i + w - 1]) / w
// the sum slides by adding the new reading and removing the old one, so each
// reading is touched twice instead of w times (rounding error can add up on
// very long data, summing every window again from scratch would avoid that)
PyObject *movingAverage(PyObject *self, PyObject *args)
{
    PyObject *seq, *list, *item;
    double *arr;
    double sum = 0.0;
    Py_ssize_t window, n, i;

    if (!PyArg_ParseTuple(args, "On", &seq, &window))
        return NULL;

    arr = toArray(seq, &n);

    if (arr == NULL)
        return NULL;

    if (window < 1 || window > n)
    {
        free(arr);
        PyErr_SetString(PyExc_ValueError, "window must be between 1 and the number of readings");
        return NULL;
    }

    list = PyList_New(n - window + 1);

    if (list == NULL)
    {
        free(arr);
        return NULL;
    }

    for (i = 0; i < window; i++)
        sum += arr[i];

    for (i = 0; i <= n - window; i++)
    {
        if (i > 0)
            sum += arr[i + window - 1] - arr[i - 1];

        item = PyFloat_FromDouble(sum / window);

        if (item == NULL)
        {
            Py_DECREF(list);
            free(arr);
            return NULL;
        }

        PyList_SET_ITEM(list, i, item);
    }

    free(arr);

    return list;
}

// O(n) time, O(n) extra space for the copy
// maximum difference = max - min
PyObject *maximumDifference(PyObject *self, PyObject *args)
{
    PyObject *seq;
    double *arr;
    Py_ssize_t n, i;
    double low, high;

    if (!PyArg_ParseTuple(args, "O", &seq))
        return NULL;

    arr = toArray(seq, &n);

    if (arr == NULL)
        return NULL;

    low = arr[0];
    high = arr[0];

    for (i = 1; i < n; i++)
    {
        if (arr[i] < low)
            low = arr[i];
        if (arr[i] > high)
            high = arr[i];
    }

    free(arr);

    return PyFloat_FromDouble(high - low);
}

// O(n) time (two passes), O(n) extra space for the copy
// mean = (x[0] + ... + x[n - 1]) / n, plain summation in a double
PyObject *sensorSummary(PyObject *self, PyObject *args)
{
    PyObject *seq;
    double *arr;
    Py_ssize_t n, i, above = 0;
    double low, high, total = 0.0, mean;

    if (!PyArg_ParseTuple(args, "O", &seq))
        return NULL;

    arr = toArray(seq, &n);

    if (arr == NULL)
        return NULL;

    low = arr[0];
    high = arr[0];

    for (i = 0; i < n; i++)
    {
        total += arr[i];

        if (arr[i] < low)
            low = arr[i];
        if (arr[i] > high)
            high = arr[i];
    }

    mean = total / n;

    for (i = 0; i < n; i++)
    {
        if (arr[i] > mean)
            above++;
    }

    free(arr);

    // Py_BuildValue makes new objects and the dict owns them
    return Py_BuildValue("{s:n,s:d,s:d,s:d,s:n}",
                         "samples", n,
                         "minimum", low,
                         "maximum", high,
                         "average", mean,
                         "above_average", above);
}

PyMethodDef methods[] = {
    {"filter_readings", filterReadings, METH_VARARGS, "filter_readings(data, minimum, maximum): readings inside the range"},
    {"count_outliers", countOutliers, METH_VARARGS, "count_outliers(data, minimum, maximum): readings outside the range"},
    {"moving_average", movingAverage, METH_VARARGS, "moving_average(data, window): average of every window"},
    {"maximum_difference", maximumDifference, METH_VARARGS, "maximum_difference(data): max minus min"},
    {"sensor_summary", sensorSummary, METH_VARARGS, "sensor_summary(data): dict of basic statistics"},
    {NULL, NULL, 0, NULL}};

struct PyModuleDef moduleDef = {
    PyModuleDef_HEAD_INIT,
    "sensor_processor",
    "sensor data filtering and analysis written in C",
    -1,
    methods};

PyMODINIT_FUNC PyInit_sensor_processor(void)
{
    return PyModule_Create(&moduleDef);
}
