import ubelt as ub
import socket
import platform
import sys


class ProcessContext:
    """
    Context manager to track the context under which a result was computed

    Example:
        >>> from json_benchmarks.benchmarker.process_context import *  # NOQA
        >>> self = ProcessContext()
        >>> obj = self.start().stop()
        >>> print('obj = {}'.format(ub.repr2(obj, nl=2)))
    """

    def __init__(self, name=None, args=None, config=None):
        if args is None:
            args = sys.argv

        self.obj = {
            'type': 'process_context',
            'name': name,
            'args': args,
            'config': config,
            'machine': None,
            'start_timestamp': None,
            'stop_timestamp': None,
        }

    def _timestamp(self):
        import datetime
        timestamp = datetime.datetime.utcnow().replace(tzinfo=datetime.timezone.utc).isoformat()
        timestamp = timestamp.replace(':', '')
        # timestamp = ub.timestamp()
        return timestamp

    def _hostinfo(self):
        return {
            'host': socket.gethostname(),
            'user': ub.Path(ub.userhome()).name,
            # 'cwd': os.getcwd(),
        }

    def _osinfo(self):
        uname_system, _, uname_release, uname_version, _, uname_processor = platform.uname()
        return {
            'os_name': uname_system,
            'os_release': uname_release,
            'os_version': uname_version,
            'arch': uname_processor,
        }

    def _pyinfo(self):
        return {
            'py_impl': platform.python_implementation(),
            'py_version': sys.version.replace("\n", ""),
        }

    def _meminfo(self):
        import psutil
        # TODO: could collect memory info at start and stop and intermediate
        # stages.  Here we just want info that is static wrt to the machine.
        # For now, just get the total available.
        svmem_info = psutil.virtual_memory()
        return {
            'mem_total': svmem_info.total,
        }

    def _cpuinfo(self):
        import cpuinfo
        _cpu_info = cpuinfo.get_cpu_info()
        cpu_info = {
            'cpu_brand': _cpu_info['brand_raw'],
        }
        return cpu_info

    def _machine(self):
        return ub.dict_union(
            self._hostinfo(),
            self._meminfo(),
            self._cpuinfo(),
            self._osinfo(),
            self._pyinfo(),
        )

    def start(self):
        self.obj.update({
            'machine': self._machine(),
            'start_timestamp': self._timestamp(),
            'stop_timestamp': None,
        })
        return self

    def stop(self):
        self.obj.update({
            'stop_timestamp': self._timestamp(),
        })
        return self.obj

    def __enter__(self):
        return self.start()

    def __exit__(self, a, b, c):
        self.stop()
