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
