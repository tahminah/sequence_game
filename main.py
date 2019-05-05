
import termios, fcntl, sys, os, time
import string, random


class CharInput:

    def __init__(self, length, in_=sys.stdin, _sleep=0.01):
        if in_ is None:
            in_ = sys.stdin
        self.in_ = in_
        self.fd = in_.fileno()
        self.length = length
        self._i = 0
        self.sleep = _sleep

    def __enter__(self):
        fd = self.fd
        self.oldterm = termios.tcgetattr(fd)
        newattr = termios.tcgetattr(fd)
        newattr[3] = newattr[3] & ~termios.ICANON & ~termios.ECHO
        termios.tcsetattr(fd, termios.TCSANOW, newattr)

        self.oldflags = fcntl.fcntl(fd, fcntl.F_GETFL)
        fcntl.fcntl(fd, fcntl.F_SETFL, self.oldflags | os.O_NONBLOCK)
        
        return self

    def __exit__(self, *args):
        fd = self.fd
        termios.tcsetattr(fd, termios.TCSANOW, self.oldterm)
        fcntl.fcntl(fd, fcntl.F_SETFL, self.oldflags)

    def __iter__(self):
        return self

    def __next__(self):
        if self._i == self.length:
            raise StopIteration()
        ch = None
        in_ = self.in_
        while ch in (None, ''):
            ch = in_.read(1)
            time.sleep(self.sleep)
        self._i += 1
        return ch


class CharWithTimestampDifferenceInput(CharInput):

    def __init__(self, *args, **kwargs):
        self._start = None
        self._ts_on_start = kwargs.pop('timestamp_on_start', False)
        self._diff_type = kwargs.pop('diff_type', 'from_adjacent')
        assert self._diff_type in ('from_adjacent', 'from_start')
        super().__init__(*args, **kwargs)

    def __next__(self):
        i = super().__next__()
        now = time.time()
        if self._start is None:
            self._start = now
            return i, (now if self._ts_on_start else None)
        r = i, now - self._start
        if self._diff_type == 'from_adjacent':
            self._start = now
        return r


class Sequence:

    @classmethod
    def random(cls, length, diff=(0.2, 1), chars=string.digits+string.ascii_lowercase, **kwargs):
        l = []
        range_lower_bound, range_upper_bound = sorted(diff)
        range_diff = range_upper_bound - range_lower_bound
        for i in range(length):
            item = random.choice(chars)
            if i == 0:
                l.append((item, None))
                continue
            diff = range_lower_bound + random.random() * range_diff
            l.append((item, diff))

        return cls(l, **kwargs)

    @classmethod
    def input(cls, length, prompt=None, _input_cls=CharWithTimestampDifferenceInput):
        if prompt is not None:
            print(prompt)
        with _input_cls(length) as input:
            input_sequence = Sequence(input)
        return input_sequence

    def __init__(self, seq):
        self.seq = list(seq)

    def compare(self, seq, inaccuracy=0.2, verbose=False, check_chars=False) -> bool:
        l = []
        for index, ((i, diff), (i2, diff2)) in enumerate(zip(self.seq, seq)):
            if index == 0:
                continue
            item = (i == i2, abs(diff - diff2))
            l.append(item)
            # if abs(diff - diff2) > self.inaccuracy:
            #     _i += 1

        if check_chars:
            ffunc = lambda x: x[1] <= inaccuracy and x[0]
        else:
            ffunc = lambda x: x[1] <= inaccuracy

        iil = len(list(filter(ffunc, l)))
        il = len(self.seq)-1
        if verbose:
            print(*map(lambda x: '{:.4f}({})'.format(x[1], '\u2713' if ffunc(x) else '\u2715' ), l), sep=' -> ')
            print('{}/{}'.format(iil, il))
        if iil < il:
            return False
        return True

    def __iter__(self):
        return iter(self.seq)

    def __str__(self):
        return str(self.seq)

    def __repr__(self):
        return repr(self.seq)

    def play(self, f=print):
        for key, sleep in self.seq:
            if sleep is not None:
                time.sleep(sleep)
            f(key)


def play_func(key):
    print(key)
    os.system('( speaker-test -t sine -f 1400 -l 1 2>&1 1>/dev/null)& pid=$!; sleep 0.08; kill -9 $pid')


length = int(input('sequence length: '))

input_sequence = Sequence.random(length)

# input_sequence = Sequence.input(length, prompt='input sequence: ')
print('ok!')

time.sleep(0.3)
print('playing sequence...')
time.sleep(0.5)
input_sequence.play(f=play_func)
print()
time.sleep(1)

print('ok, now repeat:')

with CharWithTimestampDifferenceInput(length) as input:
    match = input_sequence.compare(input, verbose=True)

    if not match:
        print('bad!')
    else:
        print('cool!')


