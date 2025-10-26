from threading import Thread, Barrier
import ujson
import random

def test_list_thread_safety():
    matrix = [[i + j * 100 for i in range(100)] for j in range(100)]
    barrier = Barrier(10)
    def test():
        barrier.wait()
        for _ in range(1000):
            row = random.randint(0, 99)
            col = random.randint(0, 99)
            matrix[row][col] = random.randint(0, 10000)
        serialized = ujson.dumps(matrix)
        deserialized = ujson.loads(serialized)
        assert all([len(row) == 100 for row in deserialized])
        assert all([deserialized[i][j] <= 10000 for i in range(100) for j in range(100)])

    threads = [Thread(target=test) for _ in range(10)]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()


def test_dict_thread_safety():
    data = {f'key_{i}': i for i in range(1000)}
    barrier = Barrier(10)
    def test():
        barrier.wait()
        for _ in range(1000):
            key = f'key_{random.randint(0, 999)}'
            data[key] = random.randint(0, 10000)
        serialized = ujson.dumps(data)
        deserialized = ujson.loads(serialized)
        assert all([deserialized[f'key_{i}'] <= 10000 for i in range(1000)])

    threads = [Thread(target=test) for _ in range(10)]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()

def test_mixed_thread_safety():
    mixed_data = {
        'numbers': [i for i in range(1000)],
        'mapping': {f'key_{i}': i for i in range(1000)},
        'nested': [{'id': i, 'value': i * 2} for i in range(1000)]
    }
    barrier = Barrier(10)
    def test():
        barrier.wait()
        for _ in range(1000):
            index = random.randint(0, 999)
            mixed_data['numbers'][index] = random.randint(0, 10000)
            mixed_data['mapping'][f'key_{index}'] = random.randint(0, 10000)
            mixed_data['nested'][index]['value'] = random.randint(0, 20000)
        serialized = ujson.dumps(mixed_data)
        deserialized = ujson.loads(serialized)
        assert all([deserialized['numbers'][i] <= 10000 for i in range(1000)])
        assert all([deserialized['mapping'][f'key_{i}'] <= 10000 for i in range(1000)])
        assert all([deserialized['nested'][i]['value'] <= 20000 for i in range(1000)])

    threads = [Thread(target=test) for _ in range(10)]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()