import os, subprocess

def batchExpand():
    inp_dir = "X-Tests Abbreviated"
    out_dir = "Y-Tests Expanded"
    
    os.makedirs(out_dir, exist_ok=True)
    
    cpp_files = [f for f in os.listdir(inp_dir) if f.endswith(".cpp")]
    if not cpp_files:
        print(f"No .cpp files found in '{inp_dir}'.")
        return
    
    print(f"Found {len(cpp_files)} file(s). Starting expansion...")
    for f in cpp_files:
        inp_path = os.path.join(inp_dir, f)
        out_path = os.path.join(out_dir, f)
        try:
            res = subprocess.run(["oj-bundle", inp_path], capture_output=True, check=True, encoding="utf-8", text=True)
            with open(out_path, "w", encoding="utf-8") as out_file:
                out_file.write(res.stdout)
            print(f"  {"\033[92m"}[SUCCESS]{"\033[0m"} {f}")
        except subprocess.CalledProcessError as e:
            print(f"  {"\033[91m"}[FAILED]{"\033[0m"}  {f}.")
            print(f"  Error details:\n{e.stderr.strip()}")        
    print("Expansion complete!")

if __name__ == "__main__":
    batchExpand()