import subprocess

def main():
    clean()
    compile()
    modes = [0, 1]
    logx_sizes = [16, 20, 24]
    y_sizes = [4, 16, 64]
    ms = [1, 4, 16, 64]
    threads = 4
    outputs = {}
    for mode in modes:
        outputs[mode] = {}
        for logx_size in logx_sizes:
            outputs[mode][logx_size] = {}
            for y_size in y_sizes:
                outputs[mode][logx_size][y_size] = {}
                for m in ms:
                    if m != 1 and logx_size != 20:
                        continue
                    run(mode, logx_size, y_size, m, threads)
                    outputs[mode][logx_size][y_size][m] = collect(m)
                    clean()
    tables = create_tables(outputs)
    generate_pdf(tables) # sudo apt install -y texlive-latex-base texlive-latex-extra

def clean():
    print('Cleaning directory ... ', end='', flush=True)
    output = subprocess.run(['make', 'clean'], capture_output=True, text=True)
    if output.returncode != 0:
        print('FAILED')
        print(output.stderr, end='')
        exit(1)
    print('OK')

def collect(m):
    print('Collecting results ... ', end='', flush=True)
    output = subprocess.run(['python3', 'summary.py', f'{m}'], capture_output=True, text=True)
    if output.returncode != 0:
        print('FAILED')
        print(output.stderr, end='')
        exit(1)
    print('OK')
    o = [line.split() for line in output.stdout.split('\n')]
    d = {
        'comp_time'      : { 'sender': [o[ 3][1], o[ 3][2]], 'receiver': [o[ 4][1], o[ 4][2]], 'total': [o[ 5][1], o[ 5][2]] },
        'comm_cost'      : { 'sender': [o[ 9][1], o[ 9][2]], 'receiver': [o[10][1], o[10][2]], 'total': [o[11][1], o[11][2]] },
        'comm_time_10g'  : { 'sender': [o[15][1], o[15][2]], 'receiver': [o[16][1], o[16][2]], 'total': [o[17][1], o[17][2]] },
        'comm_time_100m' : { 'sender': [o[21][1], o[21][2]], 'receiver': [o[22][1], o[22][2]], 'total': [o[23][1], o[23][2]] },
        'total_time_10g' : { 'sender': [o[27][1], o[27][2]], 'receiver': [o[28][1], o[28][2]], 'total': [o[29][1], o[29][2]] },
        'total_time_100m': { 'sender': [o[33][1], o[33][2]], 'receiver': [o[34][1], o[34][2]], 'total': [o[35][1], o[35][2]] }
    }
    return d

def compile():
    print('Compiling protocol ... ', end='', flush=True)
    output = subprocess.run(['make', 'protocol'], capture_output=True, text=True)
    if output.returncode != 0:
        print('FAILED')
        print(output.stderr, end='')
        exit(1)
    print('OK')

def create_tables(outputs):
    print('Creating tables ... ', end='', flush=True)
    tables = []
    tables.append(create_table2(outputs))
    tables.append(create_table3(outputs))
    tables.append(create_table456(outputs, x=20))
    tables.append(create_table456(outputs, x=16))
    tables.append(create_table456(outputs, x=24))
    print('OK')
    return tables

def generate_pdf(tables):
    print('Generating PDF ... ', end='', flush=True)
    with open('main.tex', 'w') as f:
        f.write(begin_document())
        for table in tables:
            f.write(table)
            f.write('\n')
        f.write(end_document())
        f.write('\n')
    output = subprocess.run(['pdflatex', 'main.tex'], capture_output=True, text=True)
    if output.returncode != 0:
        print('FAILED')
        print(output.stderr, end='')
        exit(1)
    # rename main.pdf to artifact-evaluation.pdf
    output = subprocess.run(['mv', 'main.pdf', 'artifact-evaluation.pdf'], capture_output=True, text=True)
    print('OK')

def run(mode, logx_size, y_size, m, threads):
    print(f'Running protocol with mode={mode}, log2|X|={logx_size}, |Y|={y_size}, m={m}, threads={threads} ... ', end='', flush=True)
    output = subprocess.run(['./protocol.exe', f'{mode}', f'{logx_size}', f'{y_size}', f'{m}', f'{threads}'], capture_output=True, text=True)
    if output.returncode != 0:
        print('FAILED')
        print(output.stderr, end='')
        exit(1)
    print('OK')

def create_table2(outputs):
    x = 20
    tag = 'comp_time'
    s = r'''
\begin{table*}[htbp]
    \vspace{-1.5cm}
    \fontsize{10}{11}\selectfont
    \centering
    \begin{tabular}{c|c|c|ccc|ccc}
        \hline
        \multirow{2}*{$|X|$} &
        \multirow{2}*{$|Y|$} &
        \multirow{2}*{$m$} &
        \multicolumn{3}{c|}{Fast Setup} &
        \multicolumn{3}{c}{Fast Intersection} \\
    
        {} & {} & {} &
        \sender{} pre & \sender{} online & \receiver{} online &
        \sender{} pre & \sender{} online & \receiver{} online \\
        
        \hline
        
        '''
    for y in [4, 16, 64]:
        for m in [1, 4, 16, 64]:
            if m == 1:
                if y == 4:
                    s += r'\multirow{12}*{$\mathscr{L} \cdot 2^{' + str(x) + r'}$} & \multirow{4}*{' + str(y) + r'} & ' + str(m) + r' &' + '\n'
                else:
                    s += r'        \cline{2-9}' + '\n'
                    s += r'        {} & \multirow{4}*{' + str(y) + r'} & ' + str(m) + r' &' + '\n'
            else:
                s += r'        {} & {} & ' + str(m) + r' &' + '\n'
            for mode in [0, 1]:
                data = outputs[mode][x][y][m][tag]
                s += f'        {data["sender"][0]} & {data["sender"][1]} & {data["receiver"][1]} ' + (r"\\" if mode else r"&") + '\n'
            s += '\n'
    
    
    s += r'''        \hline
    \end{tabular}
    \caption{Computation time (s) for two configurations of our PSI protocol, \textit{Fast Setup} and \textit{Fast Intersection}, considering a target \sender{}'s set size $|X| = \mathscr{L} \cdot 2^{20}$, several sizes of \receiver{}'s set $|Y| = \{4, 16, 64\}$, and various numbers of recurrent set intersections $m = \{1, 4, 16, 64\}$.}
    \label{tab:runtime}
\end{table*}
'''
    return s

def create_table3(outputs):
    x = 20
    s = r'''
\newcolumntype{C}[1]{>{\centering\arraybackslash}p{#1}}

\begin{table*}[htbp]
    \fontsize{10}{11}\selectfont
    \centering
    \begin{tabular}{C{1.5cm}|C{1.0cm}|C{1.0cm}|C{1.5cm}C{1.5cm}|C{1.5cm}C{1.5cm}}
        \hline
        \multirow{2}*{$|X|$} &
        \multirow{2}*{$|Y|$} &
        \multirow{2}*{$m$} &
        \multicolumn{2}{c|}{Fast Setup} &
        \multicolumn{2}{c}{Fast Intersection} \\
    
        {} & {} & {} &
        \sender{}$\rightarrow$\receiver{} & \receiver{}$\rightarrow$\sender{} &
        \sender{}$\rightarrow$\receiver{} & \receiver{}$\rightarrow$\sender{} \\
        
        \hline
        '''
    tag = 'comm_cost'
    for y in [4, 16, 64]:
        for m in [1, 4, 16, 64]:
            if m == 1:
                if y == 4:
                    s += r'\multirow{12}*{$\mathscr{L} \cdot 2^{' + str(x) + r'}$} & \multirow{4}*{' + str(y) + r'} & ' + str(m) + r' &' + '\n'
                else:
                    s += r'        \cline{2-7}' + '\n'
                    s += r'        {} & \multirow{4}*{' + str(y) + r'} & ' + str(m) + r' &' + '\n'
            else:
                s += r'        {} & {} & ' + str(m) + r' &' + '\n'
            for mode in [0, 1]:
                data = outputs[mode][x][y][m][tag]
                s += f'        {float(data["sender"][0]) + float(data["sender"][1]):.2f} & {data["receiver"][1]} ' + (r"\\" if mode else r"&") + '\n'
    s += r'''
        \hline \hline
    \end{tabular}

    \begin{tabular}{C{1.5cm}|C{1.0cm}|C{1.0cm}|C{1.5cm}C{1.5cm}|C{1.5cm}C{1.5cm}}
        \multirow{2}*{$|X|$} &
        \multirow{2}*{$|Y|$} &
        \multirow{2}*{$m$} &
        \multicolumn{2}{c|}{Fast Setup} &
        \multicolumn{2}{c}{Fast Intersection} \\
    
        {} & {} & {} &
        10 Gbps & 100 Mbps &
        10 Gbps & 100 Mbps \\
        
        \hline
        '''
    tags = ['comm_time_10g', 'comm_time_100m']
    for y in [4, 16, 64]:
        for m in [1, 4, 16, 64]:
            if m == 1:
                if y == 4:
                    s += r'\multirow{12}*{$\mathscr{L} \cdot 2^{' + str(x) + r'}$} & \multirow{4}*{' + str(y) + r'} & ' + str(m) + r' &' + '\n'
                else:
                    s += r'        \cline{2-7}' + '\n'
                    s += r'        {} & \multirow{4}*{' + str(y) + r'} & ' + str(m) + r' &' + '\n'
            else:
                s += r'        {} & {} & ' + str(m) + r' &' + '\n'
            for mode in [0, 1]:
                data1 = outputs[mode][x][y][m][tags[0]]
                data2 = outputs[mode][x][y][m][tags[1]]
                s += f'        {float(data1["sender"][0]) + float(data1["total"][1]):.2f} & {float(data2["sender"][0]) + float(data2["total"][1]):.2f} ' + (r"\\" if mode else r"&") + '\n'
    s += r'''
        \hline
    \end{tabular}
    \caption{Communication cost (MiB) for two configurations of our PSI protocol, \textit{Fast Setup} and \textit{Fast Intersection}, considering a target \sender{}'s set size $|X| = \mathscr{L} \cdot 2^{20}$ and several sizes of \receiver{}'s set $|Y| = \{4, 16, 64\}$, where we vary the number of recurrent set intersections $m = \{1, 4, 16, 64\}$. We assume a Round-Trip Time (RTT) delay of 0.2ms and 80ms for 10 Gbps and 100 Mbps network speeds, respectively.}
    \label{tab:communication}
\end{table*}
'''
    return s

def create_table456(outputs, x):
    fs = outputs[0][x]
    fi = outputs[1][x]
    m = 1
    s = r'''\setlength{\tabcolsep}{2.9pt}

\begin{table*}[t]
    \fontsize{10}{11}\selectfont
    \centering
    \begin{tabular}{|c|c|r|c|c|c|c|r|ccc|}
        \cline{4-4}\cline{9-11}
        \multicolumn{1}{c}{} &
        \multicolumn{1}{c}{} &
        {} &
        \multirow{2}*{$|X| = \mathscr{L} \cdot 2^{''' + str(x) + r'''}$} &
        \multicolumn{1}{c}{} &
        \multicolumn{1}{c}{} &
        \multicolumn{1}{c}{} &
        {} &
        \multicolumn{3}{c|}{$|Y|$} \\

        \multicolumn{1}{c}{} &
        \multicolumn{1}{c}{} &
        {} &
        {} &
        \multicolumn{1}{c}{} &
        \multicolumn{1}{c}{} &
        \multicolumn{1}{c}{} &
        {} & 4 & 16 & 64 \\
        \cline{1-4}\cline{6-11}

        \multirow{8}*{One-time} &
        \multirow{2}*{Computation} &
        Fast Setup & ''' + fs[4][m]['comp_time']['sender'][0] + r''' &
        {} &
        \multirow{8}*{Recurrent} &
        \multirow{2}*{Computation} &
        Fast Setup & ''' + f'{fs[4][m]["comp_time"]["total"][1]} & {fs[16][m]["comp_time"]["total"][1]} & {fs[64][m]["comp_time"]["total"][1]}' + r'''\\

        {} & {} & Fast Intersection & ''' + fi[4][m]['comp_time']['total'][0] + r'''&
        {} &
        {} & {} & Fast Intersection & ''' + f'{fi[4][m]["comp_time"]["total"][1]} & {fi[16][m]["comp_time"]["total"][1]} & {fi[64][m]["comp_time"]["total"][1]}' + r'''\\
        \cline{2-4}\cline{7-11}


        {} &
        \multirow{2}*{Communication} &
        Fast Setup & ''' + fs[4][m]['comm_cost']['total'][0] + r''' &
        {} &
        {} &
        \multirow{2}*{Communication} &
        Fast Setup & ''' + f'{fs[4][m]["comm_cost"]["total"][1]} & {fs[16][m]["comm_cost"]["total"][1]} & {fs[64][m]["comm_cost"]["total"][1]}' + r'''\\

        {} & {} & Fast Intersection & ''' + fi[4][m]['comm_cost']['total'][0] + r'''&
        {} &
        {} & {} & Fast Intersection & ''' + f'{fi[4][m]["comm_cost"]["total"][1]} & {fi[16][m]["comm_cost"]["total"][1]} & {fi[64][m]["comm_cost"]["total"][1]}' + r'''\\
        \cline{2-4}\cline{7-11}


        {} &
        \multirow{2}*{\begin{minipage}{1.76cm}{\centering Total time (10 Gbps)}\end{minipage}} &
        Fast Setup & ''' + fs[4][m]['total_time_10g']['total'][0] + r''' &
        {} &
        {} &
        \multirow{2}*{\begin{minipage}{1.76cm}{\centering Total time (10 Gbps)}\end{minipage}} &
        Fast Setup & ''' + f'{fs[4][m]["total_time_10g"]["total"][1]} & {fs[16][m]["total_time_10g"]["total"][1]} & {fs[64][m]["total_time_10g"]["total"][1]}' + r'''\\

        {} & {} & Fast Intersection & ''' + fi[4][m]['total_time_10g']['total'][0] + r''' &
        {} &
        {} & {} & Fast Intersection & ''' + f'{fi[4][m]["total_time_10g"]["total"][1]} & {fi[16][m]["total_time_10g"]["total"][1]} & {fi[64][m]["total_time_10g"]["total"][1]}' + r'''\\
        \cline{2-4}\cline{7-11}
        
        
        {} &
        \multirow{2}*{\begin{minipage}{1.76cm}\centering{Total time (100 Mbps)}\end{minipage}} &
        Fast Setup & ''' + fs[4][m]['total_time_100m']['total'][0] + r''' &
        {} &
        {} &
        \multirow{2}*{\begin{minipage}{1.76cm}\centering{Total time (100 Mbps)}\end{minipage}} &
        Fast Setup & ''' + f'{fs[4][m]["total_time_100m"]["total"][1]} & {fs[16][m]["total_time_100m"]["total"][1]} & {fs[64][m]["total_time_100m"]["total"][1]}' + r'''\\

        {} & {} & Fast Intersection & ''' + fi[4][m]['total_time_100m']['total'][0] + r''' &
        {} &
        {} & {} & Fast Intersection & ''' + f'{fi[4][m]["total_time_100m"]["total"][1]} & {fi[16][m]["total_time_100m"]["total"][1]} & {fi[64][m]["total_time_100m"]["total"][1]}' + r'''\\
     \cline{1-4}\cline{6-11}
        
    \end{tabular}
    \caption{Total time (in seconds) for two configurations of our PSI protocol, \textit{Fast Setup} and \textit{Fast Intersection}, considering a target \sender{}'s set size $|X| = \mathscr{L} \cdot 2^{20}$ and several sizes of \receiver{}'s set $|Y| = \{4, 16, 64\}$, where we evaluate the one-time costs and recurrent costs of performing set intersections.}
    \label{tab:round_trip''' + str(x) + r'''}
\end{table*}
'''
    return s

def begin_document():
    return r'''\documentclass{article}

\usepackage[utf8]{inputenc}
\usepackage{graphicx}
\usepackage{textcomp}
\usepackage{soul}
\usepackage{amssymb}
\usepackage{amsthm}
\usepackage{mathrsfs}
\usepackage{mathtools}
\usepackage{multicol}
\usepackage{multirow}
\usepackage{tabularx}
\usepackage{arydshln}
\usepackage[top=1cm, bottom=1cm, left=1cm, right=1cm]{geometry}
\newcommand{\receiver}{$\mathcal{R}$}
\newcommand{\sender}{$\mathcal{S}$}
\newcommand{\senc}[1]{\overline{#1}}
\newcommand{\renc}[1]{\widehat{#1}}

\pagestyle{plain}

\begin{document}

\title{Artifact Evaluation}

\author{}
\date{}
\maketitle

\setcounter{table}{1}
'''

def end_document():
    return r'\end{document}'

if __name__ == '__main__':
    main()