#!/usr/bin/env python3

import socket
import time
import subprocess
import signal
import os

class DatabaseTester:
    def __init__(self, host='localhost', port=1111):
        self.host = host
        self.port = port
        self.server_process = None
    
    def start_server(self, db_file=".save"):
        """Start the mini_db server"""
        # Clean up any existing database file
        if os.path.exists(db_file):
            os.remove(db_file)
        
        # Start server
        self.server_process = subprocess.Popen(
            ['./mini_db', str(self.port), db_file],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Wait for "ready" message
        try:
            line = self.server_process.stdout.readline().strip()
            if line != "ready":
                raise Exception(f"Server didn't start properly. Got: {line}")
            time.sleep(0.1)  # Give server a moment to fully initialize
            print("✓ Server started successfully")
            return db_file
        except Exception as e:
            self.cleanup()
            raise Exception(f"Failed to start server: {e}")
    
    def stop_server(self):
        """Stop the server gracefully with SIGINT"""
        if self.server_process:
            self.server_process.send_signal(signal.SIGINT)
            self.server_process.wait(timeout=5)
            self.server_process = None
    
    def cleanup(self):
        """Clean up resources"""
        if self.server_process:
            self.server_process.terminate()
            try:
                self.server_process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.server_process.kill()
            self.server_process = None
    
    def send_command(self, sock, command):
        """Send a command and receive response"""
        sock.send(f"{command}\n".encode())
        response = sock.recv(1024).decode().strip()
        return response
    
    def test_persistent_connection(self):
        """Test multiple commands in a single persistent connection"""
        print("\n=== Testing persistent connection (multiple commands in one session) ===")
        
        try:
            # Create a single persistent connection
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((self.host, self.port))
            
            commands_and_expected = [
                ("POST A B", "0"),
                ("POST B C", "0"), 
                ("GET A", "0 B"),
                ("GET C", "1"),
                ("DELETE A", "0"),
                ("DELETE C", "1"),
                ("UNKNOWN_COMMAND", "2")
            ]
            
            results = []
            for command, expected in commands_and_expected:
                response = self.send_command(sock, command)
                success = response == expected
                status = "✓" if success else "✗"
                print(f"{status} {command} -> '{response}' (expected: '{expected}')")
                results.append(success)
            
            sock.close()
            
            if all(results):
                print("✓ All persistent connection tests passed!")
                return True
            else:
                print("✗ Some persistent connection tests failed!")
                return False
                
        except Exception as e:
            print(f"✗ Persistent connection test failed: {e}")
            return False
    
    def test_multiple_connections(self):
        """Test multiple separate connections"""
        print("\n=== Testing multiple separate connections ===")
        
        commands_and_expected = [
            ("POST X Y", "0"),
            ("GET X", "0 Y"),
            ("DELETE X", "0"),
            ("GET X", "1")
        ]
        
        results = []
        for command, expected in commands_and_expected:
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.connect((self.host, self.port))
                response = self.send_command(sock, command)
                sock.close()
                
                success = response == expected
                status = "✓" if success else "✗"
                print(f"{status} {command} -> '{response}' (expected: '{expected}')")
                results.append(success)
                
            except Exception as e:
                print(f"✗ Failed to test {command}: {e}")
                results.append(False)
        
        if all(results):
            print("✓ All separate connection tests passed!")
            return True
        else:
            print("✗ Some separate connection tests failed!")
            return False
    
    def test_persistence_across_restarts(self, db_file):
        """Test data persistence across server restarts"""
        print("\n=== Testing persistence across server restarts ===")
        
        try:
            # Add some data
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((self.host, self.port))
            response = self.send_command(sock, "POST PERSIST_KEY PERSIST_VALUE")
            sock.close()
            
            if response != "0":
                print(f"✗ Failed to add data before restart: {response}")
                return False
            
            print("✓ Data added before restart")
            
            # Stop server gracefully (should save data)
            self.stop_server()
            print("✓ Server stopped gracefully")
            
            # Restart server
            self.server_process = subprocess.Popen(
                ['./mini_db', str(self.port), db_file],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            line = self.server_process.stdout.readline().strip()
            if line != "ready":
                raise Exception(f"Server restart failed. Got: {line}")
            time.sleep(0.1)
            
            print("✓ Server restarted successfully")
            
            # Test if data persisted
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((self.host, self.port))
            response = self.send_command(sock, "GET PERSIST_KEY")
            sock.close()
            
            if response == "0 PERSIST_VALUE":
                print("✓ Data persistence test passed!")
                return True
            else:
                print(f"✗ Data persistence test failed: got '{response}', expected '0 PERSIST_VALUE'")
                return False
                
        except Exception as e:
            print(f"✗ Persistence test failed: {e}")
            return False

def main():
    tester = DatabaseTester()
    
    try:
        print("🚀 Starting database tests...")
        
        # Start server
        db_file = tester.start_server()
        
        # Run all tests
        test_results = []
        test_results.append(tester.test_persistent_connection())
        test_results.append(tester.test_multiple_connections())
        test_results.append(tester.test_persistence_across_restarts(db_file))
        
        # Summary
        print(f"\n{'='*50}")
        passed = sum(test_results)
        total = len(test_results)
        
        if passed == total:
            print(f"🎉 All tests passed! ({passed}/{total})")
        else:
            print(f"❌ Some tests failed ({passed}/{total})")
            
        return passed == total
        
    except Exception as e:
        print(f"❌ Test setup failed: {e}")
        return False
    finally:
        tester.cleanup()
        # Clean up database file
        if os.path.exists(".save"):
            os.remove(".save")

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)