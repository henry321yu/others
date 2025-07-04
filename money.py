import time

stash0 = 16666
stashh0 = 43554
k = 0.11111111
k2 = 0.0269
times = 7
extra = 0
fix = 0
stash = stash0
stashh = stashh0
j = 29


print(f"stash:{stash:.0f}元")
print(f"raise:{k*100:.2f}%")
print(f"stash2:{stashh:.0f}元")
print(f"raise2:{k2*100:.2f}%")
# print(f"bonus:{extra:.0f}元")

print(f"{j+fix} year, {stash + stashh:.0f}元, {(stash + stashh)*12:.0f}元")
for i in range(times):
	stash += stash * k
	stashh += stashh * k2
	if i % 1 == 0:
		j += 1
		print(f"{j+fix} year, {stash + stashh:.0f}元, {(stash + stashh)*12:.0f}元")

print(f"stash:{stash:.0f}元 (gain:{(stash-stash0)/stash0*100:.2f}%)")
print(f"stash2:{stashh:.0f}元 (gain:{(stashh-stashh0)/stashh0*100:.2f}%)")

time.sleep(10)