# Test conditional statements: passing and failing conditions

x = 10
y = 5

# Passing condition (true)
if x > y:
    print("x is greater than y")

# Failing condition (false) - this should not print
if x < y:
    print("This should not print")

# If-else with passing condition
if x == 10:
    print("x equals 10")
else:
    print("x does not equal 10")

# If-else with failing condition
if x == 20:
    print("x equals 20")
else:
    print("x does not equal 20")

# Complex condition (passing)
if x > 0 and y < 10:
    print("Both conditions are true")

# Complex condition (failing)
if x < 0 or y > 10:
    print("This should not print")

# Boolean literals
flag = True
if flag:
    print("Flag is true")

flag = False
if not flag:
    print("Flag is false")

# Good for loop
nums = [1, 2, 3, 4, 5]
sum = 0
for num in nums:
    sum += num
print("Sum of nums:", sum)
