import random
import time
import sensor_processor as sp

n = 1000000
data = [random.uniform(0.0, 100.0) for i in range(n)]
low = 20.0
high = 80.0
window = 50


def timeIt(func, *args):
    start = time.perf_counter()
    func(*args)
    return time.perf_counter() - start


def pyFilter(d, lo, hi):
    return [x for x in d if lo <= x <= hi]


def pyOutliers(d, lo, hi):
    count = 0
    for x in d:
        if x < lo or x > hi:
            count += 1
    return count


def pyMoving(d, w):
    result = []
    total = sum(d[:w])
    result.append(total / w)
    for i in range(w, len(d)):
        total += d[i] - d[i - w]
        result.append(total / w)
    return result


def pyDifference(d):
    return max(d) - min(d)


def pySummary(d):
    mean = sum(d) / len(d)
    above = 0
    for x in d:
        if x > mean:
            above += 1
    return {"samples": len(d), "minimum": min(d), "maximum": max(d), "average": mean, "above_average": above}


tests = [
    ("filter_readings", pyFilter, sp.filter_readings, (data, low, high)),
    ("count_outliers", pyOutliers, sp.count_outliers, (data, low, high)),
    ("moving_average", pyMoving, sp.moving_average, (data, window)),
    ("maximum_difference", pyDifference, sp.maximum_difference, (data,)),
    ("sensor_summary", pySummary, sp.sensor_summary, (data,)),
]

print("readings:", n)
print("%-20s %10s %10s %8s" % ("function", "python", "C", "speedup"))

for name, pyFunc, cFunc, args in tests:
    py = min(timeIt(pyFunc, *args) for i in range(3))
    c = min(timeIt(cFunc, *args) for i in range(3))
    print("%-20s %9.4fs %9.4fs %7.1fx" % (name, py, c, py / c))
