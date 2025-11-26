import sqlite3
import cv2
import numpy as np
import os
import json
import struct

# Path to your database and output folder
CONFIG_PATH = "../config/default_config.json"

with open(CONFIG_PATH, "r") as f:
    cfg = json.load(f)

DB_PATH = os.path.join("..", cfg["logger"]["db_path"])
OUTPUT_FOLDER = os.path.join("..", cfg["visualizer"]["output_path"])

os.makedirs(OUTPUT_FOLDER, exist_ok=True)



def deserialize_keypoints(blob: bytes):
    """
    Deserialize keypoints + descriptors stored as BLOB in Logger DB.
    Matches EXACTLY the binary format in ipc_utils.hpp (C++).
    """

    if blob is None or len(blob) < 9:
        return [], None

    offset = 0

    # ---- Helper functions ----
    def read_u32():
        nonlocal offset
        v = struct.unpack_from("<I", blob, offset)[0]
        offset += 4
        return v

    def read_i32():
        nonlocal offset
        v = struct.unpack_from("<i", blob, offset)[0]
        offset += 4
        return v

    def read_f32():
        nonlocal offset
        v = struct.unpack_from("<f", blob, offset)[0]
        offset += 4
        return v

    # ---- HEADER ----
    N = read_u32()          # number of keypoints
    D = read_u32()          # descriptor length
    desc_type = blob[offset]  # 0=float32, 1=uint8
    offset += 1

    keypoints = []
    descriptors = None

    # Prepare descriptor matrix
    if N > 0 and D > 0:
        if desc_type == 0:
            descriptors = np.zeros((N, D), dtype=np.float32)
        else:
            descriptors = np.zeros((N, D), dtype=np.uint8)

    # ---- KEYPOINT LOOP ----
    for i in range(N):
        x = read_f32()
        y = read_f32()
        _size = read_f32()
        _angle = read_f32()
        _response = read_f32()
        _octave = read_i32()
        _class_id = read_i32()

        kp = cv2.KeyPoint(
            x=x, y=y, size=_size, angle=_angle,
            response=_response, octave=_octave, class_id=_class_id)
        keypoints.append(kp)

        # ---- Descriptors ----
        if D > 0:
            if desc_type == 0:
                # Float descriptors
                for di in range(D):
                    descriptors[i, di] = read_f32()
            else:
                # Uint8 descriptors
                descriptors[i, :] = np.frombuffer(blob, dtype=np.uint8, count=D, offset=offset)
                offset += D

    return keypoints, descriptors


def main():
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    cursor.execute("SELECT id, path, kp_blob FROM images")
    BASE_FOLDER = "../"

    for row in cursor.fetchall():
        img_id, img_path, kpt_blob = row
        img_full_path = os.path.join(BASE_FOLDER, img_path)

        img = cv2.imread(img_full_path)
        if img is None:
            print(f"Failed to load {img_path}")
            continue

        keypoints, descriptors = deserialize_keypoints(kpt_blob)

        # Draw rich keypoints
        img_kp = cv2.drawKeypoints(
            img,
            keypoints,
            None,
            flags=cv2.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS
        )

        out_path = os.path.join(OUTPUT_FOLDER, f"visualised_{img_id}.png")
        cv2.imwrite(out_path, img_kp)
        print(f"Saved Visualized keypoints to {out_path}")

    conn.close()
    print("Visualization Complete!")


if __name__ == "__main__":
    main()
