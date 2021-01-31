# pounce
HTTP benchmark utility

build
-----

     wget https://github.com/fredrikwidlund/pounce/releases/download/v1.0.0/pounce-1.0.0.tar.gz
     tar fxz pounce-1.0.0.tar.gz
     cd pounce-1.0.0
     ./configure
     make
     sudo make install

run
---

     pounce http://127.0.0.1
     requests 1767450 rps, success 100.00%, latency 38.58us/90.45us/971.71us/8.29us, usage 44.67% of 111.90Ghz
