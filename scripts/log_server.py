import socket
import datetime
import sys

# Configuration
UDP_IP = "0.0.0.0"  # Listen on all available interfaces
UDP_PORT = 18194    # Standard UDPTTY port

# ANSI Colors for premium look
class Colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def print_header():
    print(f"{Colors.BOLD}{Colors.OKBLUE}=================================================={Colors.ENDC}")
    print(f"{Colors.BOLD}{Colors.OKBLUE}   TyraCraft PS2 Remote Log Server (UDP)         {Colors.ENDC}")
    print(f"{Colors.OKCYAN}   Listening on: {UDP_IP}:{UDP_PORT}                     {Colors.ENDC}")
    print(f"{Colors.OKCYAN}   Press Ctrl+C to stop                          {Colors.ENDC}")
    print(f"{Colors.BOLD}{Colors.OKBLUE}=================================================={Colors.ENDC}")

def start_server():
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind((UDP_IP, UDP_PORT))
    except Exception as e:
        print(f"{Colors.FAIL}Error: Could not bind to port {UDP_PORT}. {e}{Colors.ENDC}")
        sys.exit(1)

    print_header()

    while True:
        try:
            # Set timeout to allow KeyboardInterrupt to be caught on Windows
            sock.settimeout(1.0)
            try:
                data, addr = sock.recvfrom(2048)
            except socket.timeout:
                continue

            timestamp = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
            message = data.decode('utf-8', errors='replace').strip()
            
            # Smart formatting based on content
            color = Colors.ENDC
            prefix = "[LOG]"
            
            if "ERROR" in message.upper() or "FAIL" in message.upper():
                color = Colors.FAIL
                prefix = "[ERR]"
            elif "WARN" in message.upper():
                color = Colors.WARNING
                prefix = "[WRN]"
            elif "SUCCESS" in message.upper() or "PASSED" in message.upper():
                color = Colors.OKGREEN
                prefix = "[OK ]"
            elif "INIT" in message.upper():
                color = Colors.OKBLUE
                prefix = "[SYS]"

            print(f"{Colors.OKCYAN}[{timestamp}]{Colors.ENDC} {Colors.BOLD}{addr[0]}{Colors.ENDC} {color}{prefix} {message}{Colors.ENDC}")
            
        except KeyboardInterrupt:
            print(f"\n{Colors.WARNING}Stopping Log Server...{Colors.ENDC}")
            break
        except Exception as e:
            print(f"{Colors.FAIL}Unexpected error: {e}{Colors.ENDC}")

    sock.close()

if __name__ == "__main__":
    # Enable ANSI colors on Windows if possible
    if sys.platform == "win32":
        import os
        os.system('color')
        
    start_server()
