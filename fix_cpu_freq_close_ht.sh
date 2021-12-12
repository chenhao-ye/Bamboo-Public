TARGET_FREQ="2600000"
		for x in /sys/devices/system/cpu/*/cpufreq; do
			echo "$TARGET_FREQ" | sudo tee "$x/scaling_min_freq" # > /dev/null 2>&1
			echo "$TARGET_FREQ" | sudo tee "$x/scaling_max_freq" # > /dev/null 2>&1
		done

echo off | sudo tee /sys/devices/system/cpu/smt/control
