#!/usr/bin/env python

# © 2019 ported to python 2/3 by Michael Lazar
# © 1994-1997,2013 by Jeffrey C. Ollie
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import re
import crypt
import socket
from subprocess import Popen, PIPE

version = '0.4'

digits = re.compile(b'^-?[0-9]+$')


class CSSOError(Exception):
    pass


class Encryptor(object):
    """Class that packages up the CCSO password encryption. This allows
    logging into the server without passing the password over the network
    in the clear."""

    ROTORSZ = 256
    MASK = 255
    b32 = 4294967296
    b31 = 2147483648

    def __init__(self, password):
        """Uses the password to initialize the encryptor state."""

        self.n1 = 0
        self.n2 = 0

        self.t1 = list(range(self.ROTORSZ))
        self.t2 = [0] * self.ROTORSZ
        self.t3 = [0] * self.ROTORSZ

        buffer = crypt.crypt(password, password)

        buffer_length = len(buffer)
        if buffer_length <= 0:
            raise CSSOError('no password supplied?')

        seed = 123

        for i in range(buffer_length):
            seed = (seed * ord(buffer[i]) + i) % self.b32

        for i in range(self.ROTORSZ):
            seed = (5 * seed + ord(buffer[i % buffer_length])) % self.b32
            if seed >= self.b31:
                sign = -1
                signed_seed = seed - self.b32
            else:
                sign = 1
                signed_seed = seed
            random = sign * int(abs(signed_seed) % 65521)
            k = self.ROTORSZ - 1 - i
            ic = (random & self.MASK) % (k + 1)
            random = (random >> 8) & self.MASK
            temp = self.t1[k]
            self.t1[k] = self.t1[ic]
            self.t1[ic] = temp
            if self.t3[k] != 0:
                continue
            if k == 0:
                raise CSSOError('[0] can\t %% by zero. k=%i' % k)
            ic = (random & self.MASK) % k
            while self.t3[ic] != 0:
                if k == 0:
                    raise CSSOError('[1] can\'t %% by zero. k=%i' % k)
                ic = (ic + 1) % k
            self.t3[k] = ic
            self.t3[ic] = k
        for i in range(self.ROTORSZ):
            self.t2[self.t1[i] & self.MASK] = i

    def _encode(self, cr):
        """Translate a list of bytes to a printable string."""
        data = self._encode_byte(len(cr))

        ts = cr[:3]
        cr = cr[3:]
        while len(ts) == 3:
            c1 = int(ts[0] / 4)
            c2 = int((ts[0] % 4) * 16 + (int(ts[1] / 16) % 16))
            c3 = int((ts[1] % 16) * 4 + (int(ts[2] / 64) % 4))
            c4 = int((ts[2] % 64))
            data = data + self._encode_byte(c1) + self._encode_byte(c2) + \
                self._encode_byte(c3) + self._encode_byte(c4)
            ts = cr[:3]
            cr = cr[3:]

        return data

    def _encode_byte(self, c):
        """Translate a byte to a printable character."""
        return chr((c % 64) + ord('#'))

    def encrypt(self, plain_str):
        """Encrypt a plaintext string."""
        cr = []

        for i in range(len(plain_str)):
            x = ord(plain_str[i]) + self.n1
            x = self.t1[x & self.MASK] + self.n2
            x = self.t3[x & self.MASK] - self.n2
            x = self.t2[x & self.MASK] - self.n1
            x = x & self.MASK
            cr.append(x)
            self.n1 = (self.n1 + 1) % self.ROTORSZ
            if self.n1 == 0:
                self.n2 = (self.n2 + 1) % self.ROTORSZ
        crypt_str = self._encode(cr)
        return crypt_str


class CCSO(object):
    """Superclass that implements the generic behavior for interacting with
    the CCSO qi (query interpreter). Instances of this class should not be
    created directly. Instead, a subclass should be created that adds
    'read_line' and 'write_line' methods."""

    def __init__(self):
        """Generic initialization. Makes sure that qi is in the state that
        we want and gathers some information about the installation."""
        self.alias = None
        self.siteinfo = {}
        self.write_line(b'set verbose=off')
        response = self._get_response()
        while response[0] < 200:
            response = self._get_response()
        if response[0] != 200:
            raise CSSOError(response[0], response[3])
        self.write_line(b'siteinfo')
        response = self._get_response()
        while response[0] < 200:
            if response[0] == -200:
                self.siteinfo[response[2]] = response[3]
            response = self._get_response()
        if b'mailbox' not in self.siteinfo:
            self.siteinfo[b'mailbox'] = b'email'

    def write_line(self, data):
        raise NotImplementedError

    def read_line(self):
        raise NotImplementedError

    def _get_response(self):
        """Private method that reads and parses the response from qi. See
        the documentation that comes with qi for a more complete description
        of the format of responses."""
        line = b''
        while not line:
            line = self.read_line()
        i1 = line.find(b':')
        if i1 == -1:
            raise CSSOError('response does not match format "%s"' % line)
        result_code = int(line[:i1])
        i2 = line.find(b':', i1 + 1)
        if i2 == -1:
            entry_index = None
            field_name = None
            text = line[i1 + 1:].strip()
        else:
            if digits.match(line[i1 + 1:i2].strip()) != -1:
                entry_index = int(line[i1 + 1:i2])
                i3 = line.find(b':', i2 + 1)
                if i3 == -1:
                    field_name = None
                    text = line[i2 + 1:].strip()
                else:
                    field_name = line[i2 + 1:i3].strip()
                    text = line[i3 + 1:].strip()
            else:
                entry_index = None
                field_name = line[i1 + 1:i2].strip()
                text = line[i2 + 1:].strip()
        return result_code, entry_index, field_name, text

    def query(self, data):
        """Send a query to qi. The string that is passed should be exactly
        as you would send it to qi, minus the 'query' keyword and and 'return'
        clause. The 'query' keyword and 'return all' are added
        automatically."""
        q = b'query ' + data.strip() + b' return all'
        self.write_line(q)
        currentindex = None
        currententry = {}
        lastfield = None
        entries = []
        response = self._get_response()
        while response[0] < 200:
            if response[0] == -200:
                if response[1] != currentindex:
                    if currentindex is not None:
                        entries.append(currententry)
                    currententry = {}
                    currentindex = response[1]
                    lastfield = None
                if response[2] == b'':
                    currententry[lastfield] = currententry[lastfield] + \
                                              b'\n' + \
                                              response[3]
                else:
                    currententry[response[2]] = response[3]
                    lastfield = response[2]
            response = self._get_response()
        if response[0] >= 300:
            raise CSSOError(response[0], response[3])
        entries.append(currententry)
        return entries

    def login(self, alias, password):
        """Log into the server so that updates can be made. Passwords are
        not sent across the network in the clear."""
        q = b'login ' + alias.strip()
        self.write_line(q)
        response = self._get_response()
        while response[0] < 200:
            response = self._get_response()
        if response[0] == 200:
            self.alias = alias
            return alias
        elif response[0] == 301:
            c = Encryptor(password)
            length, ciphertext = c.encrypt(response[3])
            self.write_line(b'answer ' + ciphertext)
            response = self._get_response()
            while response[0] < 200:
                response = self._get_response()
            if response[0] >= 400:
                raise CSSOError(response[0], response[3])
            self.alias = alias
            return alias
        else:
            raise CSSOError(response[0], response[3])

    def logout(self):
        """Log out of the server so that changes can no longer be made
        to the database."""
        q = b'logout'
        self.write_line(q)
        response = self._get_response()
        while response[0] < 200:
            response = self._get_response()
        if response[0] >= 400:
            raise CSSOError(response[0], response[3])
        self.alias = None

    def othercmd(self, data):
        """Send a raw command to qi and return the response as a list of
        4-tuples."""
        q = data.strip()
        self.write_line(q)
        responses = []
        response = self._get_response()
        while response[0] < 200:
            responses.append(response)
            response = self._get_response()
        responses.append(response)
        return responses

    def get_email(self, data):
        """Return a list (name, email address) tuples that result from
        the supplied query.
        The algorithm for get_email is taken from messages posted to
        the info-ph mailing list by Steve Dorner <sdorner@qualcomm.com>
        and Scott M. Ballew <smb@pern.cc.purdue.edu>.
        Message-ID's: <9411102104.AA14717@pern.cc.purdue.edu>
                      <v03000728aae842fd09b2@[192.17.16.12]>
                          <v0300073baae6af3eb78f@[192.17.16.12]>
        """
        mailbox = self.siteinfo[b'mailbox']
        mailfield = self.siteinfo[b'mailfield']
        responses = self.query(data)
        addresses = []
        if self.siteinfo.get(b'maildomain', b'') != b'':
            maildomain = self.siteinfo[b'maildomain']
            for entry in responses:
                if entry.get(mailbox, b'') != b'':
                    addresses.append({
                        'name': entry[b'name'],
                        'email': (entry[mailfield] + b'@' + maildomain)})
        else:
            for entry in responses:
                if entry.get(mailfield, b'') != b'':
                    addresses.append({
                        'name': entry[b'name'],
                        'email': entry[mailfield]})
        return addresses

    def close(self):
        """Send the quit command to qi. The subclass will have to worry
        about closing any sockets, files, or pipes."""
        self.write_line(b'quit')


class Local(CCSO):
    """Subclass of CCSO that implements a connection to qi running as
    a subprocess on the local system. This has the benefit that you
    don't need to log in to make changes to the database."""

    def __init__(self, command='/usr/local/sbin/qi'):
        p = Popen(command + ' -q', shell=True, stdin=PIPE, stdout=PIPE,
                  close_fds=True)
        self.read, self.write = (p.stdin, p.stdout)
        CCSO.__init__(self)

    def read_line(self):
        return self.read.readline()

    def write_line(self, line):
        self.write.write(line + b'\n')
        self.write.flush()

    def close(self):
        CCSO.close(self)
        self.write.close()
        self.read.close()


class Network(CCSO):
    """Subclass of CCSO that implements a connection over the network to
    qi running out of inetd on a remote system."""

    def __init__(self, host='localhost', port=105):
        self.host = host
        self.port = port

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host, self.port))
        CCSO.__init__(self)

    def read_line(self):
        line = b''
        ch = self.sock.recv(1)
        while ch != b'\n':
            line = line + ch
            ch = self.sock.recv(1)
        if line[-2:] == b'\r\n':
            return line[:-2] + b'\n'
        else:
            return line

    def write_line(self, line):
        self.sock.send(line + b'\r\n')

    def close(self):
        CCSO.close(self)
        self.sock.close()
