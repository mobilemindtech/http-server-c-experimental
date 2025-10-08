# http-server-c-experimental
Experimental HTTP Server in C 

## criar build dir
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build -j$(nproc)

## rodar os testes (usando ctest)
cd build
ctest -V          # verbose; retcode != 0 se algum teste falhar

## ou executar o binário de teste diretamente
./tests/test_server


## Benchmark

#### server blocking

```
$ bombardier -l http://localhost:8080/test/test -c 1000 -d 30s -t 5s

Bombarding http://localhost:8080/test/test for 30s using 1000 connection(s)
[================================================================================================================================================] 30s
Done!
Statistics        Avg      Stdev        Max
  Reqs/sec      1452.79    2055.47    6405.86
  Latency         2.36s     16.15s      2.52m
  Latency Distribution
     50%    23.73ms
     75%    24.63ms
     90%    27.28ms
     95%   642.58ms
     99%      1.82m
  HTTP codes:
    1xx - 0, 2xx - 44115, 3xx - 0, 4xx - 0, 5xx - 0
    others - 468
  Errors:
    dial tcp [::1]:8080: connect: connection refused - 396
    the server closed connection before returning the first response byte. Make sure the server returns 'Connection: close' response header before closing the connection - 72
  Throughput:    57.53KB/s
```
