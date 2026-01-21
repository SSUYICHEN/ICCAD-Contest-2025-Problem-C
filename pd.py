import matplotlib.pyplot as plt

time = [10, 13, 16, 19, 23, 25, 28, 31, 34, 37, 39, 42, 45, 48, 51]
score = [51.921, 84.695, 107.5, 128.85, 139.21, 149.15, 155.29, 168.57, 174.95, 182.01, 188.85, 195.68, 197.94, 200.26, 202.91]
timebase = [37]
scorebase = [127.19]
plt.plot(time, score, marker='o')
plt.plot(timebase, scorebase, marker='^')

plt.xlabel('Time(sec)')
plt.ylabel('Score')
plt.legend(['Ours','Reference'])
plt.title('Score vs Time')
plt.grid(True)
plt.show()
plt.savefig("pd.png")