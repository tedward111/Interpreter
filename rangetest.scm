(define range
  (lambda (n)
    (if (zero? n)
        (quote ())
        (append (range (+ n -1)) (list n)))))
(range 10000)
