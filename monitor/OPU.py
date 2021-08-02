import threading

def print_test():
    print("Test")

    test_timer()

def test_timer():
    timer = threading.Timer(3, print_test)
    timer.start()


if __name__ == "__main__":    
    test_timer()