from leapc_cffi import ffi, libleapc

connection_ptr = ffi.new("LEAP_CONNECTION*")
result = libleapc.LeapCreateConnection(ffi.NULL, connection_ptr)
if result != 0:
    print("Failed to create connection")
    exit(1)
connection = connection_ptr[0]

result = libleapc.LeapOpenConnection(connection)
if result != 0:
    print("Failed to open connection")
    exit(1)

print("Leap Motion connection opened!")