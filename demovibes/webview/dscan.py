from django.conf import settings
import os
import subprocess
import re
import logging

def is_configured():
    program = getattr(settings, 'DEMOSAUCE_SCAN', False)
    if program:
        return True
    else:
        return False

class ScanFile:
    file = ''
    bitrate = 0
    length = 0
    samplerate = 0
    loopiness = 0
    readable = False
    __replaygain = None
    
    def __init__(self, file):
        #some checks to catch bad configuration/wrong handling
        if not is_configured():
            logging.error("ScanFile got called without being enabled in configuration")
            return
        program = getattr(settings, 'DEMOSAUCE_SCAN', False)
        if not os.path.isfile(program):
            logging.error("ScanFile can't find scan tool at %s. check your config" % program) 
        if not file: 
            logging.error("ScanFile got called with no file which indicates a bug in our code")
            return
        if not os.path.isfile(str(file)):
            logging.error("ScanFile can't find %s. this is likely our fault" % str(file)) 
            return

        self.file = str(file)        
        path = os.path.dirname(program)
        p = subprocess.Popen([program, '--no-replaygain', self.file], stdout = subprocess.PIPE, cwd = path)
        output = p.communicate()[0]
        if p.returncode != 0:
            logging.warn("scan doesn't like %s" % self.file)
            return
            
        self.readable = True	
        
        bitrate = re.compile(r'bitrate:(\d*\.?\d+)')
        length = re.compile(r'length:(\d*\.?\d+)')
        samplerate = re.compile(r'samplerate:(\d*\.?\d+)')
        loopiness = re.compile(r'loopiness:(\d*\.?\d+)')
        
        self.length = float(length.search(output).group(1))

        samplerate_match = samplerate.search(output)
        if samplerate_match:
            self.samplerate = float(samplerate_match.group(1))

        bitrate_match = bitrate.search(output)
        if bitrate_match:
            self.bitrate = float(bitrate_match.group(1))

        loop_match = loopiness.search(output)
        if loop_match:
            self.loopiness = float(loop_match.group(1))
        
    def replaygain(self):
        if not self.readable:
            return 0
            
        if not self.__replaygain:        
            program = getattr(settings, 'DEMOSAUCE_SCAN', False)
            path = os.path.dirname(program)
            p = subprocess.Popen([program, self.file], stdout=subprocess.PIPE, cwd = path)
            output = p.communicate()[0]
            repgain = re.compile(r'replaygain:(-?\d*\.?\d+)')
            self.__replaygain = float(repgain.search(output).group(1))
        
        return self.__replaygain
        