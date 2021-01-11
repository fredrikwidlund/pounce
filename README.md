# pounce
HTTP benchmark utility

pounce is designed to push the limit of HTTP benchmarking tools

Build
-----

     wget https://github.com/fredrikwidlund/pounce/releases/download/v1.0.0/pounce-1.0.0.tar.gz
     tar fxz pounce-1.0.0.tar.gz
     ./configure
     make

Run
---

     ./bin/pounce http://127.0.0.1
     requests 1767450 rps, success 100.00%, latency 38.58us/90.45us/971.71us/8.29us, usage 44.67% of 111.90Ghz

Compare
-------
     wrk -t40 -c160 http://127.0.0.1
     Running 10s test @ http://127.0.0.1
       40 threads and 160 connections
       Thread Stats   Avg      Stdev     Max   +/- Stdev
         Latency   495.42us    1.14ms  20.08ms   90.10%
         Req/Sec    33.09k     6.63k   94.35k    68.78%
       13199984 requests in 10.10s, 1.66GB read
     Requests/sec: 1306921.73
     Transfer/sec:    168.26MB
 
