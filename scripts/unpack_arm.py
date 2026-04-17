import argparse
from pathlib import Path
from PIL import Image

def unpack_arm(input_path: str, output_dir: str = None):
    input_file = Path(input_path)
    if not input_file.exists():
        print(f"Error: File '{input_path}' not found.")
        return

    if output_dir:
        out_dir = Path(output_dir)
        out_dir.mkdir(parents=True, exist_ok=True)
    else:
        out_dir = input_file.parent

    try:
        img = Image.open(input_file)
        if img.mode not in ("RGB", "RGBA"):
            img = img.convert("RGB")
        
        # Split into channels (R, G, B) and optionally A
        channels = img.split()
        r = channels[0]
        g = channels[1]
        b = channels[2]
        
        base_name = input_file.stem
        ext = input_file.suffix
        
        ao_path = out_dir / f"{base_name}_ao{ext}"
        roughness_path = out_dir / f"{base_name}_roughness{ext}"
        metallic_path = out_dir / f"{base_name}_metallic{ext}"
        
        r.save(ao_path)
        g.save(roughness_path)
        b.save(metallic_path)
        
        print(f"Successfully unpacked '{input_file.name}':")
        print(f"  - Ambient Occlusion (R) -> {ao_path.name}")
        print(f"  - Roughness (G)         -> {roughness_path.name}")
        print(f"  - Metallic (B)          -> {metallic_path.name}")
        
    except Exception as e:
        print(f"An error occurred while processing '{input_path}': {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Unpack ARM/ORM (Ambient Occlusion, Roughness, Metallic) packed texture into separate channel images."
    )
    parser.add_argument("input", help="Path to the packed texture image (e.g., texture_arm.png)")
    parser.add_argument("-o", "--output", help="Output directory for the separated textures (default: same directory as input)", default=None)
    
    args = parser.parse_args()
    unpack_arm(args.input, args.output)
