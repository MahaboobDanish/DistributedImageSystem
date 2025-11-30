import cv2
import sqlite3
import numpy as np
import json
import os
import struct

# Load Config
CONFIG_PATH = "../config/default_config.json"
with open(CONFIG_PATH, "r") as f:
    cfg = json.load(f)

DB_PATH = os.path.join("..",cfg["logger"]["db_path"])
IMAGE_ROOT = os.path.join("..",cfg["logger"]["image_save_path"])

# Load DB Records

conn = sqlite3.connect(DB_PATH)
cursor = conn.cursor()
cursor.execute("SELECT id, path, kp_blob FROM images")
records = cursor.fetchall()
conn.close()

if len(records)==0:
    raise RuntimeError("No images found in databse")

index = 0
current_keypoints = []
current_image = None

# Keypoint Deserialization

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

        # # ---- Descriptors ----
        # if D > 0:
        #     if desc_type == 0:
        #         # Float descriptors
        #         for di in range(D):
        #             descriptors[i, di] = read_f32()
        #     else:
        #         # Uint8 descriptors
        #         descriptors[i, :] = np.frombuffer(blob, dtype=np.uint8, count=D, offset=offset)
        #         offset += D

    return keypoints

# Load Image + Keypoints
BASE_FOLDER = "../"
def load_frame(i):
    global current_keypoints,current_image
    img_id, img_path, blob = records[i]
    img_full_path = os.path.join(BASE_FOLDER, img_path)
    img = cv2.imread(img_full_path)

    # img = cv2.imread(img_path)
    if img is None:
        raise RuntimeError(f"Failed to load {img_path}")
    
    keypoints = deserialize_keypoints(blob)
    vis = cv2.drawKeypoints(
        img,keypoints,None,
        flags=cv2.DRAW_MATCHES_FLAGS_DRAW_RICH_KEYPOINTS
    )

    current_keypoints=keypoints
    current_image = vis
    cv2.imshow("Interactive SIFT viewer", vis)

# Mouse Callback
def on_mouse(event, x,y,flags, param):
    if event!=cv2.EVENT_LBUTTONDOWN:
        return 
    
    min_dist = 8
    selected = None

    for kp in current_keypoints:
        dx = kp.pt[0]-x
        dy = kp.pt[1]-y
        dist = np.sqrt(dx*dx + dy*dy)
        if dist<min_dist:
            min_dist=dist
            selected = kp
    if selected:
        print("\n--- Keypoint Selected ---")
        print(f"Position : ({int(selected.pt[0])}, {int(selected.pt[1])})")
        print(f"Scale    : {selected.size:.2f}")
        print(f"Angle    : {selected.angle:.2f}")
        print(f"Response : {selected.response:.4f}")
        print(f"Octave   : {selected.octave}")

# Main Loop
cv2.namedWindow("Interactive SIFT Viewer")
cv2.setMouseCallback("Interactive SIFT Viewer", on_mouse)
load_frame(index)

while True:
    key = cv2.waitKey(0)
    if key==ord('q'):
        break
    elif key ==ord('n'):
        index = (index+1)%len(records)
        load_frame(index)
    elif key==ord('p'):
        index = (index-1)%len(records)
        load_frame(index)
cv2.destroyAllWindows()


