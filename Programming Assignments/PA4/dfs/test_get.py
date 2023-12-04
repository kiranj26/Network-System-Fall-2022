from struct import pack, unpack
import socket
import time


def main():
    user = b'Alice'
    password = b'SimplePassword'
    subdir = b'.'

    def get_header(fn):
        header_size = 4 + 8 + len(user) + 8 + len(password) + 8 + len(subdir) + 8 + len(file_name)
        h = pack('q', header_size)
        h += pack('i', 1)
        h += pack('q', len(user)) + user
        h += pack('q', len(password)) + password
        h += pack('q', len(subdir)) + subdir
        h += pack('q', len(fn)) + fn

        return h

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(('localhost', 10001))

        file_name = b'test.txt.3'
        header = get_header(file_name)
        print('header_size', len(header))
        s.sendall(header)

        # response: len(8 bytes), content(len bytes)
        file_len = unpack('q', s.recv(8))[0]
        print(s.recv(file_len))

        file_name = b'test.txt.1'
        header = get_header(file_name)
        print('header_size', len(header))
        s.sendall(header)

        # response: len(8 bytes), content(len bytes)
        file_len = unpack('q', s.recv(8))[0]
        if file_len > 0:
            print(s.recv(file_len))
        else:
            print('file not found')

        s.close()


if __name__ == '__main__':
    main()
