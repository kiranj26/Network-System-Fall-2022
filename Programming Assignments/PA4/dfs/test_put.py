from struct import pack, unpack
import socket
import time


def main():
    user = b'Alice'
    password = b'SimplePassword'
    subdir = b'test'

    command = 2

    def put_header(fc, fn):
        # note that `header_size` does not include 8 bytes of the size field!
        header_size = 4 + 8 + len(user) + 8 + len(password) + 8 + len(subdir) + 8 + len(file_name) + 8

        h = pack('q', header_size)
        h += pack('i', command)
        h += pack('q', len(user)) + user
        h += pack('q', len(password)) + password
        h += pack('q', len(subdir)) + subdir
        h += pack('q', len(fn)) + fn
        h += pack('q', len(fc))

        return h

    # print("header size: ", header_size, len(header))

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(('localhost', 8001))

        file_content = b'this is b.txt.1 file content by test_put.py'
        file_name = b'b.txt.1'
        header = put_header(file_content, file_name)
        s.sendall(header)
        s.sendall(file_content)
        time.sleep(0.5)

        file_content = b'this is b.txt.4 file content by test_put.py'
        file_name = b'b.txt.4'
        header = put_header(file_content, file_name)
        s.sendall(header)
        s.sendall(file_content)

        time.sleep(0.5)

        s.close()


if __name__ == '__main__':
    main()
