cltuner
=======
Istruzioni per la compilazione:

 mkdir build
 cd build
 cmake .. (opzionalmente con -DCLFFT_SOURCE_DIR=/path/to/clFFT/src, v. sotto)
 make

## Modulo clFFT (facoltativo)
Per fare benchmark con clFFT è necessario scaricare il codice sorgente di clFFT
e passare -DCLFFT_SOURCE_DIR=/path/to/clFFT/src a cmake.
 git clone https://github.com/clMathLibraries/clFFT.git
CLFFT_SOURCE_DIR deve puntare alla directory "src" di clFFT.
