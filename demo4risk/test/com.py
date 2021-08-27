def compute():
    a = 47168.78 * 0.2716
    b = 47161.79 * 0.01
    c = 47158.79 * 0.7184
    d = a + b + c

    print("%f + %f = %f" % (a, b, a+b))
    print("%f + %f + %f = %f" % (a, b, c, d))

if __name__ == '__main__':
    compute()        