#!/usr/bin/python3
import serial
from serial.serialutil import SerialException
import click

RESPONSE_OK = 0x01


def calc_crc(data):
    crc = 0
    for d in data:
        crc ^= d
    return crc & 0xFF


def do_write(data, address, port):

    port.write(b's')
    port.write([address & 0xFF, address >> 8])

    ack = ord(port.read(1))
    if ack != RESPONSE_OK:
        print('Error sending address')
        return False
    print('Sent base address')
    port.write(b'l')
    length = len(data)
    port.write([length & 0xFF, length >> 8])

    ack = ord(port.read(1))
    if ack != RESPONSE_OK:
        print('Error sending length')
        return False
    print('Sent length')

    chunk = data[0:512]
    data = data[512:]

    port.write(b'd')

    ack = ord(port.read(1))
    if ack != RESPONSE_OK:
        print('Error sending command')
        return False
    print('Sent command')

    written = 0

    while len(chunk) > 0:
        port.write(chunk)

        crc = calc_crc(chunk)

        ack = port.read(2)

        if ack[0] == RESPONSE_OK:
            if crc != ack[1]:
                print('Error in Receive CRC, local %d, writer:%d' %
                      (crc, ack[1]))
                port.write(bytes([0xFF]))
                return False
            else:
                port.write(bytes([0x01]))
        else:
            print('Error Writing')
            return False

        ack = port.read(2)

        if ack[0] == RESPONSE_OK:
            if crc != ack[1]:
                print('Error in Write CRC, local %d, writer:%d' %
                      (crc, ack[1]))
                port.write(bytes([0xFF]))
                return False
            else:
                port.write(bytes([0x01]))
        else:
            print('Error Writing')
            return False

        written += len(chunk)
        print('Wrote %d bytes' % written)

        chunk = data[0:512]
        data = data[512:]

    print('Write OK')
    return True


def do_read(address, length, port):
    port.write(b's')
    port.write([address & 0xFF, address >> 8])

    ack = ord(port.read(1))
    if ack != RESPONSE_OK:
        print('Error Reading')
        return False, None

    port.write(b'l')
    port.write([length & 0xFF, length >> 8])

    ack = ord(port.read(1))
    if ack != RESPONSE_OK:
        print('Error Reading')
        return False, None

    port.write(b'r')

    data = b''

    print('Starting ...')
    while len(data) < length:
        chunk = port.read(min(256, length - len(data)))
        data += chunk
        print('Read %d bytes' % len(data))

    crc = ord(port.read(1))

    local_crc = calc_crc(data)

    if local_crc != crc:
        print('CRC Error: %d - %d' % (crc, local_crc))
        return False, None

    print("Read OK")

    return True, data


def do_read_byte(port):
    port.write('+')
    return True, ord(port.read(1))


def connect(port, baudrate):
    return serial.Serial(port=port,
                         baudrate=int(baudrate),
                         timeout=60,
                         bytesize=serial.EIGHTBITS,
                         parity=serial.PARITY_NONE,
                         stopbits=serial.STOPBITS_ONE)


@click.group()
def cli():
    pass


@click.command()
@click.option('--port', required=True, help='Port Name')
@click.option('--baud', required=True, help='Baud Rate')
@click.option('--rom', required=True, help='ROM File Image')
@click.option('--address', default=0, help='Base Address')
def write(port, baud, rom, address):

    try:
        with open(rom, 'rb') as f:
            data = f.read()
    except Exception as e:
        print('Unable to open file: %s' % e)
        return 1
    print('ROM Size: %d bytes' % len(data))
    try:
        with connect(port, baud) as conn:
            print('Connection to %s success' % port)
            do_write(data, int(address), conn)
    except SerialException as e:
        print('Unable to write to serial port: %s' % e)
        return 1


@click.command()
@click.option('--port', required=True, help='Port Name')
@click.option('--baud', required=True, help='Baud Rate')
@click.option('--output', required=True, help='Output file name')
@click.option('--address', default=0, help='Base Address')
@click.option('--length', default=16384, help='Length')
def read(port, baud, output, address, length):

    contents = None
    try:
        with connect(port, baud) as conn:
            result, contents = do_read(int(address), int(length), conn)

            if not result:
                return 1
    except SerialException as e:
        print('Unable to read from serial port: %s' % e)
        return 1

    try:
        with open(output, 'wb') as f:
            f.write(contents)
    except Exception as e:
        print('Unable to open file: %s' % e)
        return 1

    return 0


@click.command()
@click.option('--port', required=True, help='Port Name')
@click.option('--baud', required=True, help='Baud Rate')
def readbyte(port, baud):

    try:
        with connect(port, baud) as conn:
            result, content = do_read_byte(conn)

            print('Result:%02x' % content)
            if not result:
                return 1
    except SerialException as e:
        print('Unable to read serial port: %s' % e)
        return 1

    return 0


@click.command()
@click.option('--port', required=True, help='Port Name')
@click.option('--baud', required=True, help='Baud Rate')
def reset(port, baud):

    try:
        with connect(port, baud) as conn:

            conn.write('0')

    except SerialException as e:
        print('Unable to read serial port: %s' % e)
        return 1

    return 0


cli.add_command(write)
cli.add_command(read)
cli.add_command(readbyte)
cli.add_command(reset)

if __name__ == '__main__':
    cli()
