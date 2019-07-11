import random

# for each of the 3 files
for i in range(3):
	# Create file
	file = open("f" + str(i) + ".txt", "w+")
	# generate the random 10 character string
	x = ''.join(random.choice('abcdefghijklmnopqrstuvwxyz') for i in range(10))
	file.write(x + '\n')
	print(x)

# generate random numbers between 1 and 42
a = random.randint(1, 43)
print(a)
b = random.randint(1, 43)
print(b)
print(a * b)
