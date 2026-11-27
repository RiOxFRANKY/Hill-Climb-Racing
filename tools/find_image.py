import json
import base64
import os
from PIL import Image

path = r'C:\Users\USER\.claude\projects\E--Projects-QtProjects-HillClimbRacingMain\949e28de-734e-4c66-8d73-c6f50a21c196.jsonl'
found_images = 0

with open(path, 'r', encoding='utf-8', errors='ignore') as f:
    for idx, line in enumerate(f):
        if 'image/png' in line or 'image/jpeg' in line or 'base64' in line:
            print(f'Line {idx} mentions image/base64, len={len(line)}')
            d = json.loads(line)
            
            def search_dict(obj, p=''):
                if isinstance(obj, dict):
                    if obj.get('type') == 'image':
                        src = obj.get('source', {})
                        print(f"  Found image block at {p}: media_type={src.get('media_type')}, data len={len(src.get('data', ''))}")
                        return src.get('data', '')
                    for k, v in obj.items():
                        res = search_dict(v, p + '.' + k)
                        if res: return res
                elif isinstance(obj, list):
                    for i, v in enumerate(obj):
                        res = search_dict(v, f'{p}[{i}]')
                        if res: return res
                return None
                
            data_b64 = search_dict(d)
            if data_b64:
                found_images += 1
                out_path = os.path.join('tools', f'recovered_image_{idx}.png')
                with open(out_path, 'wb') as img_out:
                    img_out.write(base64.b64decode(data_b64))
                with Image.open(out_path) as im:
                    print(f"  [SAVED] {out_path}: size={im.size}, mode={im.mode}")

print('Total images extracted:', found_images)
