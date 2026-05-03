def square(n):
    return n * n

nums = range(1, 6)
acc = 0
for x in nums:
    acc += square(x)

print("acc", acc)
