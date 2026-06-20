import argparse;
import struct;

if (__name__ == "__main__"):
	parser = argparse.ArgumentParser(prog="transmitter", description="Transmit Binary Blob over UART to rrisc bios");
	parser.add_argument("blob");
	parser.add_argument("-o", "--output", help="output pipe");
	args = parser.parse_args();
	bfd = open(args.blob, "rb");
	ofd = open(args.output, "wb");
	blob = bfd.read();
	blob += b"\x00" * (4 - (len(blob) % 4))
	ofd.write(struct.pack(">I", len(blob) >> 2) + blob);
	bfd.close();
	ofd.close();
