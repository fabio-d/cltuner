#!/usr/bin/env python2
import fcntl, os, sys, struct, subprocess, tempfile, termios
from collections import OrderedDict

DESCRIPTIONS = {
	'timestamp': "Time stamp for kernel launches and memory transfers",
	'gputime': "Execution time for the GPU kernel or memory copy (us, calculated as end - start)",
	'cputime': "For blocking operations, gputime + CPU overhead; for non-blocking operations, CPU overhead only",
	'ndrangesize': "Number of blocks in a grid along the X, Y and Z dimensions for a kernel launch",
	'workgroupsize': "Number of threads in a block along the X, Y and Z dimensions for a kernel launch",
	# controllare 'stasmemperblock': "Size of statically allocated shared memory per block in bytes for a kernel launch",
	'regperworkitem': "Number of registers used per thread for a kernel launch",
	'occupancy': 'Percentage of the maximum warp count, 1.0 means the chip is completely full',
	'memtransferdir': "Memory transfer direction (1 is host->device, 2 is device->host)",
	'memtransfersize': "Memory copy size in bytes",
	'streamid': "Stream ID for a kernel launch",
	'gld_incoherent': "Non-coalesced (incoherent) global memory loads",
	'gld_coherent': "Coalesced (coherent) global memory loads",
	'gld_32b': "32-byte global memory load transactions",
	'gld_64b': "64-byte global memory load transactions",
	'gld_128b': "128-byte global memory load transactions",
	'gld_request': "Global memory loads (increments per warp)", # include global, private e local memory
	'gst_incoherent': "Non-coalesced (incoherent) global memory stores",
	'gst_coherent': "Coalesced (coherent) global memory stores",
	'gst_32b': "32-byte global memory store transactions",
	'gst_64b': "64-byte global memory store transactions",
	'gst_128b': "128-byte global memory store transactions",
	'gst_request': "Global memory stores (increments per warp)", # include global, private e local memory
	'local_load': "Local memory loads",
	'local_store': "Local memory stores",
	'branch': "Branches taken by threads executing a kernel",
	'divergent_branch': "Divergent branches taken by threads executing a kernel",
	'instructions': "Instructions executed",
	'warp_serialize': "Number of thread warps that serialize on address conflicts to either shared or constant memory",
	'cta_launched': "Number of threads blocks executed"
}

FIXED_OPTIONS = [
	'timestamp', 'gridsize3d', 'threadblocksize', 'dynsmemperblock',
	'stasmemperblock', 'regperthread', 'memtransferdir', 'memtransfersize',
	#'gpustarttimestamp', 'gpuendtimestamp' <-- non necessari, c'e' gia' gputime
	#'streamid' <-- buggato, non emette output ben formato
	]

COUNTERS = [
	'gld_incoherent', 'gld_coherent', 'gld_32b', 'gld_64b', 'gld_128b', 'gld_request',
	'gst_incoherent', 'gst_coherent', 'gst_32b', 'gst_64b', 'gst_128b', 'gst_request',
	'local_load', 'local_store', 'branch', 'divergent_branch',
	'instructions', 'warp_serialize', 'cta_launched'
]

def group(elems, groupsize):
	for i in range(0, len(elems), groupsize):
		yield elems[i:i+groupsize]

def terminal_size():
	# http://stackoverflow.com/questions/566746/how-to-get-console-window-width-in-python
	h, w, hp, wp = struct.unpack('HHHH', fcntl.ioctl(sys.stdout.fileno(), termios.TIOCGWINSZ, struct.pack('HHHH', 0, 0, 0, 0)))
	return w, h

def do_pass(command, metrics):
	print("\nAnalisi di %s...\n" % ', '.join(metrics))
	with tempfile.NamedTemporaryFile('w') as conffile, tempfile.NamedTemporaryFile('r') as outfile:
		conffile.write('\n'.join(FIXED_OPTIONS + metrics))
		conffile.flush()

		environ = os.environ.copy()
		environ['COMPUTE_PROFILE'] = '1'
		#environ['COMPUTE_PROFILE_CSV'] = '1'
		environ['COMPUTE_PROFILE_CONFIG'] = conffile.name
		environ['COMPUTE_PROFILE_LOG'] = outfile.name
		subprocess.call(command, env=environ)

		for line in outfile:
			# Salta righe che non contengono dati
			if '=' not in line:
				continue

			# Parsing dei dati e conversione in dizionario
			kvpairs = [ (kv[0].strip(), kv[1].strip()) for kv in group((line.strip()[:-1].strip()).replace('=[ ',' ] ').split(' ] '), 2) ]
			yield OrderedDict(kvpairs)

try:
	term_cols, term_rows = terminal_size()
except IOError:
	term_cols = term_rows = 100000 # Numero grande

# Questa struttura verra' riempita con un dizionario per ogni evento registrato
events = []

# Si possono misurare solo 4 metriche alla volta
for metrics in group(COUNTERS, 4):
	for i, event in enumerate(do_pass(sys.argv[1:], metrics)):
		# Crea dizionario (alla prima passata non esiste ancora)
		if len(events) <= i:
			events.append(OrderedDict([]))
		# Riempi campi del dizionario
		for key, value in event.items():
			events[i][key] = value

# Stampa dei dati relativi a ciascun evento
for event in events:
	# Nome dell'evento
	print "== %s ==" % event['method']
	del event['method']

	# Altri campi, con evntuale descrizione, qualora disponibile
	for key, value in event.items():
		if key in DESCRIPTIONS:
			helptext = ' ' + DESCRIPTIONS[key]
		else:
			helptext = ''

		# Stampa una riga, troncando la descrizione se necessario
		print "  %-19s %-15s%s" % (key, value, helptext[:(int(term_cols)-38)])
