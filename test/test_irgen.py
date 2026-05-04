# Test file for IR generation
# Includes various language features to exercise the IRGenerator

# Variables and assignments
x = 10
y = 3.14
z = "hello"

# Arithmetic expressions
a = x + y * 2
b = (a - 5) // 2
c = b ** 2

# Augmented assignments
x += 5
y *= 2

# Lists
nums = [1, 2, 3, 4, 5]
names = ["alice", "bob", "charlie"]

# List operations
first = nums[0]
nums[2] = 99
combined = nums + [6, 7]

# Function definition
def factorial(n):
    if n <= 1:
        return 1
    else:
        return n * factorial(n - 1)

# Function call
fact5 = factorial(5)

# If statement
if fact5 > 100:
    print("Factorial is large:", fact5)
elif fact5 > 50:
    print("Factorial is medium:", fact5)
else:
    print("Factorial is small:", fact5)

# While loop
i = 0
total = 0
while i < len(nums):
    total += nums[i]
    i += 1

print("Sum of nums:", total)

# For loop
squares = []
for num in nums:
    squares += [num * num]

print("Squares:", squares)

# Nested structures
matrix = [[1, 2], [3, 4]]
print("Matrix element:", matrix[1][0])

# Boolean expressions
flag = True
if not flag and x > 0:
    print("Condition met")
else:
    print("Condition not met")