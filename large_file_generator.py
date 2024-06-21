import datetime

# Define the entry format
entry_format = "{}.{:02}"  # Example: "2024-06-13 12:58:44,82"

# Calculate the size of one entry in bytes (assuming ASCII encoding)
entry_size_bytes = len(entry_format.format("2024-06-13 12:58:44", 82))

# Target file size in GB
target_size_gb = 60

# Convert GB to bytes
target_size_bytes = target_size_gb * 1024 * 1024 * 1024

# Calculate the number of entries needed
num_entries = target_size_bytes // entry_size_bytes

# Function to generate the entry string
def generate_entry():
    now = datetime.datetime.now()
    return entry_format.format(now.strftime("%Y-%m-%d %H:%M:%S"), now.microsecond // 10000)

# Open a file for writing
output_file = "large_file.txt"
with open(output_file, 'w') as file:
    # Write entries until the target size is reached
    for _ in range(num_entries):
        entry = generate_entry()
        file.write(entry + '\n')

print(f"Generated {output_file} with size approximately {target_size_gb} GB.")

