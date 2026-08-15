import os
import glob
import struct

def fix_wav_headers(directory):
    for filepath in glob.glob(os.path.join(directory, '*.wav')):
        with open(filepath, 'rb+') as f:
            header = f.read(46)
            if len(header) < 46:
                continue
            
            if header[0:4] != b'RIFF' or header[8:12] != b'WAVE':
                continue
                
            # Check if data chunk is at offset 38
            if header[38:42] == b'data':
                riff_size = struct.unpack('<I', header[4:8])[0]
                data_size = struct.unpack('<I', header[42:46])[0]
                
                # If sizes are 0 or incorrect, fix them
                file_size = os.path.getsize(filepath)
                expected_riff_size = file_size - 8
                expected_data_size = file_size - 46
                
                if riff_size != expected_riff_size or data_size != expected_data_size:
                    print(f"Fixing {filepath}...")
                    
                    # Write RIFF size
                    f.seek(4)
                    f.write(struct.pack('<I', expected_riff_size))
                    
                    # Write data size
                    f.seek(42)
                    f.write(struct.pack('<I', expected_data_size))

if __name__ == '__main__':
    fix_wav_headers('assets/audio/SFX')
