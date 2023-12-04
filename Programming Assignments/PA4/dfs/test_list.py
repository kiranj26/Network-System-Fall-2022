from struct import pack, unpack
import socket
import time


def main():
    user = b'Alice'
    password = b'SimplePassword'
    subdir = b'test'

    header_size = 4 + 8 + len(user) + 8 + len(password) + 8 + len(subdir)
    header = pack('q', header_size) + pack('i', 0) + pack('q', len(user)) +\
             user + pack('q', len(password)) + password + pack('q', len(subdir)) + subdir
    print("header size: ", header_size, len(header))

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(('localhost', 10001))
        s.sendall(header)

        while True:
            fn_len = unpack('Q', s.recv(8))[0]
            if fn_len == 0:
                break
            print(s.recv(fn_len))

        s.close()


if __name__ == '__main__':
    main()
