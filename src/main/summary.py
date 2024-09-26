import os
import sys

def list_tmp_files(directory):
    tmp_files = []
    for filename in os.listdir(directory):
        if filename.endswith('.tmp'):
            tmp_files.append(filename)
    return tmp_files

def get_file_size(file_path):
    return os.path.getsize(file_path)

def main():
    # read 'm' from arguments
    if len(sys.argv) != 2:
        print("Usage: python summary.py <m>")
        return
    m = int(sys.argv[1]) # 'm' accounts for the number of recurrencies

    directory = '.'
    tmp_files = list_tmp_files(directory)
    from_sender_pre = 0
    from_sender = 0
    from_receiver = 0
    
    if tmp_files:
        # print("List of .tmp files and their sizes:")
        for file_name in tmp_files:
            file_path = os.path.join(directory, file_name)
            size = get_file_size(file_path)
            # print(f"{file_name}: {size} bytes")
            if file_name == 'encrypted_table.tmp':
                from_sender_pre += size
            elif file_name[3] == 'E':
                # print(f'From sender: {file_name}')
                from_sender += size
            elif file_name[3] == 'D' or file_name[3] == 'R':
                # print(f'From receiver: {file_name}')
                from_receiver += size
            else:
                raise ValueError(f'Unknown file: {file_name}')
    else:
        print("No .tmp files found in the directory.")
        return

    # load sender and receiver runtime from file 'runtime.log'
    with open('runtime.log', 'r') as f:
        lines = f.readlines()
        sender_runtime_pre = float(lines[0].strip()) / 1000 # conversion to seconds
        sender_runtime = float(lines[1].strip()) / 1000 / m # conversion to seconds
        receiver_runtime = float(lines[2].strip()) / 1000 / m # conversion to seconds

    # to MiB
    from_sender_pre /= 1<<20
    from_sender /= (1<<20) * m
    from_receiver /= (1<<20) * m
    total = from_sender + from_receiver

    # Round-Trip Time (RTT) in seconds
    rtt_10G = 0.0002 # 0.2 ms
    rtt_100 = 0.08   # 80 ms

    # Bandwidth in MiB/s
    bandwidth_10G = 10**4 / 8
    bandwidth_100 = 10**2 / 8

    # communication time in seconds for 10 Gbps and 100 Mbps networks
    tcp_overhead = 1.04 # around 4% overhead for TCP
    communication_time_pre_10G = tcp_overhead * from_sender_pre / bandwidth_10G + 2 * rtt_10G
    communication_time_pre_100 = tcp_overhead * from_sender_pre / bandwidth_100 + 2 * rtt_100
    communication_time_10G = tcp_overhead * total / bandwidth_10G + 2 * rtt_10G
    communication_time_100 = tcp_overhead * total / bandwidth_100 + 2 * rtt_100

    # Show results

    print('-----------------------------------')
    print(f'Computation time (s)')
    print(f'From     One-time Recurrent')
    print(f'Sender   {sender_runtime_pre: 8.2f}  {sender_runtime:8.2f}')
    print(f'Receiver     0.00  {receiver_runtime:8.2f}')
    print(f'Total    {sender_runtime_pre: 8.2f}  {sender_runtime + receiver_runtime:8.2f}')
    print('-----------------------------------')
    print(f'Communication cost (MiB)')
    print(f'From     One-time Recurrent')
    print(f'Sender   {from_sender_pre: 8.2f}  {from_sender:8.2f}')
    print(f'Receiver     0.00  {from_receiver:8.2f}')
    print(f'Total    {from_sender_pre: 8.2f}  {total:8.2f}')
    print('-----------------------------------')
    print(f'Communication time for 10 Gbps network (s)') # Assuming RTT = 0.2ms
    print(f'From     One-time Recurrent')
    print(f'Sender   {communication_time_pre_10G: 8.2f}  {communication_time_10G:8.2f}')
    print(f'Receiver     0.00  {communication_time_10G:8.2f}')
    print(f'Total    {communication_time_pre_10G: 8.2f}  {communication_time_10G:8.2f}')
    print('-----------------------------------')
    print(f'Communication time for 100 Mbps network (s)') # Assuming RTT = 80ms
    print(f'From     One-time Recurrent')
    print(f'Sender   {communication_time_pre_100: 8.2f}  {communication_time_100:8.2f}')
    print(f'Receiver     0.00  {communication_time_100:8.2f}')
    print(f'Total    {communication_time_pre_100: 8.2f}  {communication_time_100:8.2f}')
    print('-----------------------------------')
    print(f'Total time for 10 Gbps network (s)')
    print(f'From    One-time Recurrent')
    print(f'Sender   {sender_runtime_pre + communication_time_pre_10G: 8.2f}  {sender_runtime + communication_time_10G:8.2f}')
    print(f'Receiver     0.00  {receiver_runtime + communication_time_10G:8.2f}')
    print(f'Total    {sender_runtime_pre + communication_time_pre_10G: 8.2f}  {sender_runtime + receiver_runtime + communication_time_10G:8.2f}')
    print('-----------------------------------')
    print(f'Total time for 100 Mbps network (s)')
    print(f'From    One-time Recurrent')
    print(f'Sender   {sender_runtime_pre + communication_time_pre_100: 8.2f}  {sender_runtime + communication_time_100:8.2f}')
    print(f'Receiver     0.00  {receiver_runtime + communication_time_100:8.2f}')
    print(f'Total    {sender_runtime_pre + communication_time_pre_100: 8.2f}  {sender_runtime + receiver_runtime + communication_time_100:8.2f}')
    print('-----------------------------------')

if __name__ == "__main__":
    main()