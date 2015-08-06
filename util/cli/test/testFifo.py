#!/usr/bin/env python2.7

import os ;
import subprocess ;

from lib import procutils ;

'''
It makes sure that FIFO buffer size is 5.
It is running and compled within 15 seconds at least.

Result is follow as :
	[1438859950:service.c +69] CMD70 : sleep 3
	[1438859950:utilFifo.c +151] FIFO Item is empty and waiting Item ... 1
	[1438859951:utilFifo.c +151] FIFO Item is empty and waiting Item ... 2
	[1438859952:utilFifo.c +151] FIFO Item is empty and waiting Item ... 3
	[1438859953:service.c +69] CMD71 : sleep 3
	[1438859953:utilFifo.c +151] FIFO Item is empty and waiting Item ... 1
	[1438859954:utilFifo.c +151] FIFO Item is empty and waiting Item ... 2
	[1438859955:utilFifo.c +151] FIFO Item is empty and waiting Item ... 3
	[1438859956:utilFifo.c +151] FIFO Item is empty and waiting Item ... 4
	[1438859956:service.c +69] CMD72 : sleep 3
	[1438859956:utilFifo.c +151] FIFO Item is empty and waiting Item ... 1
	[1438859957:utilFifo.c +151] FIFO Item is empty and waiting Item ... 2
	[1438859958:utilFifo.c +151] FIFO Item is empty and waiting Item ... 3
	[1438859959:utilFifo.c +151] FIFO Item is empty and waiting Item ... 4
	[1438859959:service.c +69] CMD73 : sleep 5 &
	[1438859959:utilFifo.c +151] FIFO Item is empty and waiting Item ... 1
	[1438859959:service.c +69] CMD74 : sleep 5 &
	[1438859959:utilFifo.c +151] FIFO Item is empty and waiting Item ... 1
	[1438859959:service.c +69] CMD75 : sleep 5 &
	[1438859959:utilFifo.c +151] FIFO Item is empty and waiting Item ... 1
	[1438859959:service.c +69] CMD76 : sleep 5 &
	[1438859959:utilFifo.c +151] FIFO Item is empty and waiting Item ... 1
	[1438859959:service.c +69] CMD77 : sleep 5 &
	[1438859959:utilFifo.c +151] FIFO Item is empty and waiting Item ... 1
	[1438859959:service.c +69] CMD78 : sleep 5 &
	[1438859959:utilFifo.c +151] FIFO Item is empty and waiting Item ... 1
	[1438859959:service.c +69] CMD79 : sleep 5 &
	[1438859959:service.c +69] CMD80 : sleep 5 &
	[1438859959:service.c +69] CMD81 : sleep 5 &
	[1438859959:service.c +69] CMD82 : sleep 3
	[1438859962:service.c +69] CMD83 : sleep 3
	[1438859965:service.c +69] CMD84 : sleep 3
'''

DEVNULL = open(os.devnull, 'w') ;

class SysMode :
	FG = 0 ;
	BG = 1 ;

class TestFifo :
	cli = "cli"
	cmd = [] ;
	__sendCount = 1 ;
	def __init__(self, cmdFg='sleep 3', cmdBg='sleep 5') :
		if not procutils.which(self.cli) :
			self.cli = "../cli"
		# print 'CLI :', self.cli ;
		self.cmd = [ [ self.cli, 'service', 'cmd', cmdFg, 'fg' ],
							[ self.cli, 'service', 'cmd', cmdBg, 'bg' ] ] ;
	def sendSystem(self, cmd) :
		strCmd = '%0*d' % (3, self.__sendCount) + '] ';
		self.__sendCount += 1 ;
		for sc in cmd :
			strCmd += sc ;
			strCmd += " " ;
		print strCmd ;
		subprocess.call(cmd, stderr=DEVNULL) ;
	def testFifo(self) :
		print 'Run a foreground command lists :', self.cmd[SysMode.FG] ;
		print 'Run a background command lists :', self.cmd[SysMode.BG] ;
		for i in range(0, 3) :
			self.sendSystem(self.cmd[SysMode.FG]) ;
		for i in range(0, 9) :
			self.sendSystem(self.cmd[SysMode.BG]) ;
		for i in range(0, 3) :
			self.sendSystem(self.cmd[SysMode.FG]) ;


def main() :
	TestFifo().testFifo() ;


if __name__ == '__main__':
	main()

'''
CLI=`which cli`
[ -z "$CLI" ] && CLI=../cli

fRunTest() {
	# service cmd  - <cmd> [<fg|bg> <rvPath>] : execute command in svc.service.
	$CLI service cmd 'sleep 3'
	$CLI service cmd 'sleep 3'
	$CLI service cmd 'sleep 3'
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3' bg
	$CLI service cmd 'sleep 3'
	$CLI service cmd 'sleep 3'
	$CLI service cmd 'sleep 3'
}

fCheckPoint() {
	echo
	echo -e "It makes sure that FIFO buffer size is 5."
	echo -e "It is running and compled within 15 seconds at least."
	echo
}

fRunTest
fCheckPoint
'''
