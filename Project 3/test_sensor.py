import sensor_processor as sp


def attempt(label, func, *args):
    try:
        print(label, "->", func(*args))
    except Exception as e:
        print(label, "->", type(e).__name__ + ":", e)


data = [10.5, 25.0, 110.0, 35.5]

print("--- normal use ---")
attempt("filter_readings(data, 20.0, 40.0)", sp.filter_readings, data, 20.0, 40.0)
attempt("count_outliers(data, 20.0, 40.0)", sp.count_outliers, data, 20.0, 40.0)
attempt("moving_average([10, 20, 30, 40], 2)", sp.moving_average, [10.0, 20.0, 30.0, 40.0], 2)
attempt("maximum_difference([10, 25, 40, 15])", sp.maximum_difference, [10.0, 25.0, 40.0, 15.0])
attempt("sensor_summary([10, 20, 30, 40])", sp.sensor_summary, [10.0, 20.0, 30.0, 40.0])

print()
print("--- tuples and ints ---")
attempt("filter_readings(tuple)", sp.filter_readings, (10.5, 25.0, 110.0, 35.5), 20.0, 40.0)
attempt("sensor_summary(ints)", sp.sensor_summary, [1, 2, 3, 4, 5])
attempt("filter_readings(nothing in range)", sp.filter_readings, data, 200.0, 300.0)
attempt("moving_average(window = length)", sp.moving_average, [1.0, 2.0, 3.0], 3)
attempt("sensor_summary(one reading)", sp.sensor_summary, [7.5])

print()
print("--- empty input ---")
attempt("filter_readings([])", sp.filter_readings, [], 0.0, 10.0)
attempt("count_outliers([])", sp.count_outliers, [], 0.0, 10.0)
attempt("moving_average([], 1)", sp.moving_average, [], 1)
attempt("maximum_difference([])", sp.maximum_difference, [])
attempt("sensor_summary(())", sp.sensor_summary, ())

print()
print("--- invalid input ---")
attempt("sensor_summary('abc')", sp.sensor_summary, "abc")
attempt("maximum_difference(42)", sp.maximum_difference, 42)
attempt("sensor_summary({1.0, 2.0})", sp.sensor_summary, {1.0, 2.0})
attempt("maximum_difference([1.0, 'x'])", sp.maximum_difference, [1.0, "x"])
attempt("filter_readings(data, 40.0, 20.0)", sp.filter_readings, data, 40.0, 20.0)
attempt("count_outliers(data, 'a', 40.0)", sp.count_outliers, data, "a", 40.0)
attempt("moving_average(data, 0)", sp.moving_average, data, 0)
attempt("moving_average(data, -2)", sp.moving_average, data, -2)
attempt("moving_average(data, 10)", sp.moving_average, data, 10)
attempt("moving_average(data, 2.5)", sp.moving_average, data, 2.5)
