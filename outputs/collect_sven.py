import re
# \d = [0-9]
pat_tpt = "throughput= ?(\d+(\.\d+e\+\d+)?)"
pat_lat = {}
pat_lat['main'] = {}
pat_lat['main']['99'] = "(?<=p99=)[0-9]+\.[0-9]+"
pat_lat['main']['999'] = "(?<=p999=)[0-9]+\.[0-9]+"
pat_lat['sven'] = {}
pat_lat['sven']['99'] = "p99= ?(\d+)"
pat_lat['sven']['999'] = "p999= ?(\d+)"

runs = [0, 1, 2]
cc_algs = ["SILO_PRIO", "SILO"]
zipfs = [0.9, 0.99, 1.2]
workloads = ["TPCC"]
for zipf in zipfs:
    workloads.append(f"YCSB_{zipf}")
threads = [1, 2, 4, 8, 16, 32, 64]

# silo-prio-main-sven
directory = "silo-prio-main-sven"
with open("collect-main.txt", 'w') as output:
    for run in runs:
        for cc_alg in cc_algs:
            for workload in workloads:
                for thread in threads:
                    with open(f"{directory}/{run}_{cc_alg}_{workload}_{thread}_output.txt", "r") as input:
                        content = input.read()
                        tpts = re.findall(pat_tpt, content)
                        tpt = tpts[0][0] if len(tpts) > 0 else -1
                        lat99 = re.findall(pat_lat['main']['99'], content)
                        lat999 = re.findall(pat_lat['main']['999'], content)
                        if len(lat99) > 0 and len(lat999) > 0:
                            output.write(f"{tpt}\t{lat99[0]}\t{lat999[0]}\n")
                        else:
                            output.write(f"{tpt}\t\t\n")
                        if cc_alg == "SILO_PRIO":
                            if len(lat99) > 1 and len(lat999) > 1:
                                output.write(f"\t{lat99[1]}\t{lat999[1]}\n")
                            else:
                                output.write(f"\t\t\n")

# silo-prio-sven
directory = "silo-prio-sven"
with open("collect-sven.txt", 'w') as output:
    for run in runs:
        for cc_alg in cc_algs:
            for workload in workloads:
                for thread in threads:
                    with open(f"{directory}/{run}_{cc_alg}_{workload}_{thread}_output.txt", "r") as input:
                        lat99 = []
                        lat999 = []
                        tpt = ""
                        for line in input:
                            if line.startswith("[summary]"):
                                tpt = re.findall(pat_tpt, line)[0][0]
                            if line.startswith("total,"):
                                lat99.append( float(re.findall(pat_lat['sven']['99'], line)[0]) / 1000 )
                                lat999.append( float(re.findall(pat_lat['sven']['999'], line)[0]) / 1000 )
                        output.write(f"{tpt}\t{lat99[0]}\t{lat999[0]}\n")
                        if cc_alg == "SILO_PRIO":
                            output.write(f"\t{lat99[1]}\t{lat999[1]}\n")