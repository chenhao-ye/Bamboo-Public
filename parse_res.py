#take in a log file, output results(or to json) in the format of
#throughput average lat
#priorities at each lvl \n
#aborts at each lvl\n
#lats (OR short-lats long-lats)\n
import os, sys, re, os.path
import json


def parse(log_name, has_long):

    has_prio = False
    with open(log_name, "r") as log:
        
        for line in log:
            #if silo_prio, parse priority
            if 'PRIO' in line:
                has_prio = True

            #thrupt and avg lat
            if re.search('summary', line):
                #print(line.strip())

                parts = [float(s) for s in re.findall(r'-?\d+\.?\d*', line)]
                thrupt = parts[0]
                lat = parts[19]
                print(str(thrupt)+','+str(lat))
            
                cur_line=next(log)

                #priority
                if has_prio:
                    prios = list()
                    cur_prio_lvl = 0
                    while not re.search('prio_txn_cnt', cur_line):
                        cur_line=next(log)
                    cur_line=next(log)
                    #print('prio')
                    while not re.search(']', cur_line):
                        prio, cnt = re.findall(r'\d+', cur_line)
                        prios.append( (prio, cnt) )
                        #print(cur_line.strip())
                        cur_line=next(log)

                    prios_str=''
                    i = 0
                    for j in range(int(prios[-1][0])+1):
                        while i < int(prios[j][0]):
                            prios_str = prios_str+'0 '
                            i+=1
                        prios_str = prios_str+prios[j][1]+','
                        i+=1

                    print(prios_str)



                #print('aborts')
                #aborts
                abrts = list()
                while not re.search('abort_txn_cnt', cur_line):
                    cur_line=next(log)
                cur_line=next(log)
                while not re.search(']', cur_line):
                    abrt, cnt = re.findall(r'\d+', cur_line)
                    abrts.append( (abrt, cnt) )
                    #print(cur_line.strip())
                    cur_line=next(log)
                
                abrts_str=''
                i = 0
                for j in range(len(abrts)):
                    while i < int(abrts[j][0]):
                        abrts_str = abrts_str+'0 '
                        i+=1
                    abrts_str = abrts_str+abrts[j][1]+','
                    i+=1
                    
                print(abrts_str)

                
                
                
                #latencies
                #print('lats')
                lats = list()
                while not re.search(' us', cur_line):
                    cur_line=next(log)
                while cur_line != '':
                    try :
                        cur_line=next(log)
                        parts = [float(s) for s in re.findall(r'-?\d+\.?\d*', cur_line)]
                        #print(cur_line)
                        #print(parts)
                        if len(parts) == 0:
                            continue
                        lats.append(parts[-1])

                    except StopIteration:
                        #print(lats)
                        [print(l, end=',') for l in lats]
                        print('')
                        break
                
                break



    return

if __name__ == "__main__":
    if len(sys.argv) < 3 or len(sys.argv) > 4:
        print("usage: path/to/log includes_long_txn(Y/N) [path/to/out_json]")
        exit()
    log_name = sys.argv[1]
    has_long = True if 'Y' in sys.argv[2].upper() else False
    #to_json = True if len(sys.argv) == 4 else False

    parse(log_name, has_long)
