import numpy as np
import imageio
import sys
import os


def equirect_to_cubemap(input_path, output_dir, size=1024):
    """
    Converts an equirectangular HDR image to 6 cubemap faces.
    Requires numpy and imageio[freeimage].
    """
    print(f"Loading {input_path}...")
    img = imageio.v3.imread(input_path, plugin="opencv")
    h, w, _ = img.shape

    # Define the 6 faces and their orientations
    # face: (up, right, front)
    faces = {
        "px": (0, 0, -1),  # +X
        "nx": (0, 0, 1),  # -X
        "py": (0, -1, 0),  # +Y
        "ny": (0, 1, 0),  # -Y
        "pz": (1, 0, 0),  # +Z
        "nz": (-1, 0, 0),  # -Z
    }

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Precompute coordinates
    x = np.linspace(-1, 1, size)
    y = np.linspace(-1, 1, size)
    xv, yv = np.meshgrid(x, y)

    for face_name, (up_axis, right_axis, front_axis) in faces.items():
        print(f"Generating face {face_name}...")

        # Calculate 3D direction for each pixel in the cubemap face
        if face_name == "px":
            vecs = np.stack([np.ones_like(xv), -yv, -xv], axis=-1)
        elif face_name == "nx":
            vecs = np.stack([-np.ones_like(xv), -yv, xv], axis=-1)
        elif face_name == "py":
            vecs = np.stack([xv, np.ones_like(xv), yv], axis=-1)
        elif face_name == "ny":
            vecs = np.stack([xv, -np.ones_like(xv), -yv], axis=-1)
        elif face_name == "pz":
            vecs = np.stack([xv, -yv, np.ones_like(xv)], axis=-1)
        elif face_name == "nz":
            vecs = np.stack([-xv, -yv, -np.ones_like(xv)], axis=-1)

        # Normalize vectors
        norm = np.linalg.norm(vecs, axis=-1, keepdims=True)
        vecs /= norm

        # Map to equirectangular UVs
        phi = np.arctan2(vecs[..., 0], vecs[..., 2])
        theta = np.arcsin(vecs[..., 1])

        u = (phi + np.pi) / (2 * np.pi)
        v = (theta + np.pi / 2) / np.pi

        # Sample (bilinear interpolation would be better, but simple nearest for now)
        u_idx = (u * (w - 1)).astype(int)
        v_idx = ((1 - v) * (h - 1)).astype(int)

        face_img = img[v_idx, u_idx]

        output_path = os.path.join(output_dir, f"{face_name}.hdr")
        imageio.v3.imwrite(output_path, face_img)
        print(f"Saved to {output_path}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python hdr_to_cubemap.py <input.hdr> <output_dir> [size]")
    else:
        size = int(sys.argv[3]) if len(sys.argv) > 3 else 1024
        equirect_to_cubemap(sys.argv[1], sys.argv[2], size)
