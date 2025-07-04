import time

stash0 = 200000
stashh0 = 525089
k = 0.11111111
k2 = 0.0269
times = 10
extra = 0
fix = 0
stash = stash0
stashh = stashh0
j = 29


print(f"stash:{stash:.0f}元")
print(f"stash2:{stashh:.0f}元")
print(f"raise:{k*100:.2f}%")
print(f"raise2:{k2*100:.2f}%")
print(f"bonus:{extra:.0f}元")

print(f"{j+fix} year, {stash + stashh:.0f}元")
for i in range(times):
	stash += stash * k
	stashh += stashh * k2
	if i % 1 == 0:
		j += 1
		print(f"{j+fix} year, {stash + stashh:.0f}元")

time.sleep(3)