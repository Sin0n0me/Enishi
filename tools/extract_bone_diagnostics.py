"""Offline PMD/PMX bone diagnostics. Never exports geometry or source paths."""
import argparse
import json
import math
import struct
from pathlib import Path


class Reader:
    def __init__(self, stream):
        self.stream = stream
        self.size = stream.seek(0, 2)
        stream.seek(0)
        self.encoding = 'utf-8'
        self.widths = []

    def skip(self, size):
        if size < 0 or size > self.size - self.stream.tell():
            raise ValueError('Truncated or invalid binary section')
        self.stream.seek(size, 1)

    def raw(self, size):
        if size < 0 or size > self.size - self.stream.tell():
            raise ValueError('Truncated or invalid binary section')
        return self.stream.read(size)

    def unpack(self, fmt):
        values = struct.unpack('<' + fmt, self.raw(struct.calcsize('<' + fmt)))
        if any(isinstance(v, float) and not math.isfinite(v) for v in values):
            raise ValueError('Non-finite numeric value')
        return values[0] if len(values) == 1 else list(values)

    def count(self, fmt='i', minimum=1):
        value = self.unpack(fmt)
        if value < 0 or value > (self.size - self.stream.tell()) // minimum:
            raise ValueError('Invalid section count')
        return value

    def text(self, keep=False):
        size = self.count()
        if keep:
            return self.raw(size).decode(self.encoding)
        self.skip(size)

    def name(self, size):
        return self.raw(size).split(b'\0', 1)[0].decode('cp932')

    def index(self, kind):
        return self.unpack({1: 'b', 2: 'h', 4: 'i'}[self.widths[kind]])


def body_fields(r, bone, index):
    record = {'index': index, 'bone': bone}
    for key, fmt in [('group', 'B'), ('collision_mask_raw', 'H'), ('shape', 'B'),
                     ('size', '3f'), ('position', '3f'), ('rotation_xyz_radians', '3f'),
                     ('mass', 'f'), ('linear_damping', 'f'), ('angular_damping', 'f'),
                     ('restitution', 'f'), ('friction', 'f'), ('mode', 'B')]:
        record[key] = r.unpack(fmt)
    return record


def joint_fields(r, index, kind, a, b):
    record = {'index': index, 'type': kind, 'body_a': a, 'body_b': b}
    for key in ['position', 'rotation_xyz_radians', 'translation_min', 'translation_max',
                'rotation_min', 'rotation_max', 'translation_spring', 'rotation_spring']:
        record[key] = r.unpack('3f')
    return record


def read_pmd(r):
    if r.unpack('f') != 1.0:
        raise ValueError('Unsupported PMD version')
    r.skip(276)  # Model name and comment are not needed.
    r.skip(r.count('I', 38) * 38)
    r.skip(r.count('I', 2) * 2)
    r.skip(r.count('I', 70) * 70)
    bones = []
    for i in range(r.count('H', 39)):
        name = r.name(20)
        parent, tail, kind, ik_parent = r.unpack('HHBH')
        bones.append({'index': i, 'name': name, 'parent': -1 if parent == 65535 else parent,
                      'tail': tail, 'type': kind, 'ik_parent': ik_parent,
                      'position': r.unpack('3f')})
    ik = []
    for _ in range(r.count('H', 11)):
        owner, target, length, iterations, angle = r.unpack('HHBHf')
        ik.append({'owner': owner, 'target': target, 'iterations': iterations,
                   'angle': angle, 'links': [r.unpack('H') for _ in range(length)]})
    morph_count = r.count('H', 25)
    for _ in range(morph_count):
        r.skip(20)
        length = r.count('I', 16)
        r.skip(1 + length * 16)
    r.skip(r.count('B', 2) * 2)
    frames = r.count('B', 50)
    r.skip(frames * 50)
    r.skip(r.count('I', 3) * 3)
    if r.stream.tell() == r.size:
        return bones, ik, [], []
    english = r.unpack('B')
    if english not in (0, 1):
        raise ValueError('Invalid PMD English flag')
    if english:
        r.skip(276 + len(bones) * 20 + max(0, morph_count - 1) * 20 + frames * 50)
    r.skip(1000)  # Toon texture paths.
    if r.stream.tell() == r.size:
        return bones, ik, [], []
    bodies = []
    for i in range(r.count('I', 83)):
        r.skip(20)
        bone = r.unpack('H')
        bodies.append(body_fields(r, -1 if bone == 65535 else bone, i))
    joints = []
    for i in range(r.count('I', 124)):
        r.skip(20)
        a, b = r.unpack('II')
        joints.append(joint_fields(r, i, 0, a, b))
    return bones, ik, bodies, joints


def skip_pmx_geometry(r):
    for _ in range(4):
        r.text()
    for _ in range(r.count()):
        r.skip(32 + r.additional_uv * 16)
        deform = r.unpack('B')
        width = r.widths[3]
        sizes = {0: width, 1: 2 * width + 4, 2: 4 * width + 16,
                 3: 2 * width + 40, 4: 4 * width + 16}
        if deform not in sizes:
            raise ValueError('Unsupported PMX deformation type')
        r.skip(sizes[deform] + 4)
    r.skip(r.count() * r.widths[0])
    for _ in range(r.count()):
        r.text()
    for _ in range(r.count()):
        r.text()
        r.text()
        r.skip(65 + 2 * r.widths[1] + 1)
        shared = r.unpack('B')
        if shared not in (0, 1):
            raise ValueError('Invalid PMX toon flag')
        r.skip(1 if shared else r.widths[1])
        r.text()
        r.skip(4)


def read_pmx_bones(r):
    bones = []
    ik = []
    for i in range(r.count()):
        bone = {'index': i, 'name': r.text(True)}
        r.text()
        bone.update(position=r.unpack('3f'), parent=r.index(3), layer=r.unpack('i'))
        flags = r.unpack('H')
        bone['flags'] = flags
        bone['tail' if flags & 1 else 'tail_offset'] = r.index(3) if flags & 1 else r.unpack('3f')
        if flags & 0x0300:
            bone.update(inherit_parent=r.index(3), inherit_weight=r.unpack('f'))
        if flags & 0x0400:
            bone['fixed_axis'] = r.unpack('3f')
        if flags & 0x0800:
            bone.update(local_x=r.unpack('3f'), local_z=r.unpack('3f'))
        if flags & 0x2000:
            bone['external_parent'] = r.unpack('i')
        if flags & 0x0020:
            entry = {'owner': i, 'target': r.index(3), 'iterations': r.unpack('i'),
                     'angle': r.unpack('f'), 'links': []}
            for _ in range(r.count()):
                link = {'bone': r.index(3)}
                limited = r.unpack('B')
                if limited not in (0, 1):
                    raise ValueError('Invalid PMX IK limit flag')
                if limited:
                    link.update(lower=r.unpack('3f'), upper=r.unpack('3f'))
                entry['links'].append(link)
            ik.append(entry)
        bones.append(bone)
    return bones, ik


def read_pmx(r):
    version = r.unpack('f')
    if not any(abs(version - supported) < 0.00001 for supported in (2.0, 2.1)):
        raise ValueError('Unsupported PMX version')
    if r.unpack('B') != 8:
        raise ValueError('Unsupported PMX header size')
    encoding, r.additional_uv = r.unpack('BB')
    if encoding not in (0, 1) or r.additional_uv > 4:
        raise ValueError('Invalid PMX header')
    r.encoding = 'utf-16-le' if encoding == 0 else 'utf-8'
    r.widths = r.unpack('6B')
    if any(width not in (1, 2, 4) for width in r.widths):
        raise ValueError('Invalid PMX index width')
    skip_pmx_geometry(r)
    bones, ik = read_pmx_bones(r)
    # Seek over morph payloads; no vertex, UV or material values are decoded.
    w = r.widths
    sizes = {0: w[4] + 4, 1: w[0] + 12, 2: w[3] + 28,
             3: w[0] + 16, 4: w[0] + 16, 5: w[0] + 16,
             6: w[0] + 16, 7: w[0] + 16, 8: w[2] + 113,
             9: w[4] + 4, 10: w[5] + 25}
    for _ in range(r.count()):
        r.text()
        r.text()
        r.skip(1)
        kind = r.unpack('B')
        if kind not in sizes:
            raise ValueError('Unsupported PMX morph type')
        r.skip(r.count() * sizes[kind])
    for _ in range(r.count()):
        r.text()
        r.text()
        r.skip(1)
        for _ in range(r.count()):
            kind = r.unpack('B')
            if kind not in (0, 1):
                raise ValueError('Invalid PMX display type')
            r.skip(w[3] if kind == 0 else w[4])
    bodies = []
    for i in range(r.count()):
        r.text()
        r.text()
        bodies.append(body_fields(r, r.index(3), i))
    joints = []
    for i in range(r.count()):
        r.text()
        r.text()
        kind = r.unpack('B')
        if kind > 5:
            raise ValueError('Unsupported PMX joint type')
        a, b = r.index(5), r.index(5)
        joints.append(joint_fields(r, i, kind, a, b))
    return bones, ik, bodies, joints


def select_bones(bones, names, indices):
    selected = set(indices)
    for name in names:
        matches = [b['index'] for b in bones if b['name'] == name]
        if not matches:
            raise ValueError('Requested bone name not found')
        selected.update(matches)
    if any(i < 0 or i >= len(bones) for i in selected):
        raise ValueError('Requested bone index out of range')
    included = set(selected)
    for index in selected:
        seen = set()
        while index != -1:
            if index in seen or index < 0 or index >= len(bones):
                raise ValueError('Invalid or cyclic bone parent chain')
            seen.add(index)
            included.add(index)
            index = bones[index]['parent']
    return selected, included


def read_motion(path, bones, start, end):
    # VMD stores at most 15 CP932 bytes, possibly ending inside a character.
    names = {b['index']: b['name'].encode('cp932', errors='replace')[:15] for b in bones}
    if len(set(names.values())) != len(names):
        raise ValueError('Selected bone names are ambiguous in VMD')
    tracks = {name: {} for name in names.values()}
    with path.open('rb') as stream:
        r = Reader(stream)
        signature = r.raw(30).split(b'\0', 1)[0]
        if signature == b'Vocaloid Motion Data 0002':
            r.skip(20)
        elif signature == b'Vocaloid Motion Data file':
            r.skip(10)
        else:
            raise ValueError('Unsupported VMD signature')
        for _ in range(r.count('I', 111)):
            name = r.raw(15).split(b'\0', 1)[0]
            if name not in tracks:
                r.skip(96)
                continue
            frame = r.unpack('I')
            key = {'frame': frame, 'translation': r.unpack('3f'),
                   'rotation_xyzw': r.unpack('4f'), 'interpolation_raw': r.unpack('64B')}
            tracks[name][frame] = key
    result = []
    for bone in bones:
        keys = tracks[names[bone['index']]]
        frames = sorted(keys)
        before = [f for f in frames if f < start]
        after = [f for f in frames if f > end]
        keep = {f for f in frames if start <= f <= end}
        # Bracketing keys are required to interpolate at the interval boundaries.
        keep.update(before[-1:])
        keep.update(after[:1])
        result.append({'bone': bone['index'], 'keys': [keys[f] for f in sorted(keep)]})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('model', type=Path, help='Local PMD or PMX file')
    parser.add_argument('--list-bones', action='store_true', help='Print only bone indices and names')
    parser.add_argument('--bone', action='append', default=[], help='Exact bone name; repeatable')
    parser.add_argument('--bone-index', type=int, action='append', default=[], help='Bone index; repeatable')
    parser.add_argument('--include-names', action='store_true', help='Include selected bone names in JSON')
    parser.add_argument('--motion', type=Path, help='Optional local VMD file (bone keys only)')
    parser.add_argument('--frame-start', type=int, default=0)
    parser.add_argument('--frame-end', type=int, default=60)
    parser.add_argument('--output', type=Path, help='New JSON file; existing files are never overwritten')
    args = parser.parse_args()
    if not args.list_bones and (not args.output or not (args.bone or args.bone_index)):
        parser.error('Extraction requires --output and --bone or --bone-index')
    if args.frame_start < 0 or args.frame_end < args.frame_start:
        parser.error('Invalid frame interval')
    try:
        with args.model.open('rb') as stream:
            r = Reader(stream)
            magic = r.raw(3)
            if magic == b'Pmd':
                kind = 'PMD'
                bones, ik, bodies, joints = read_pmd(r)
            elif magic == b'PMX' and r.raw(1) == b' ':
                kind = 'PMX'
                bones, ik, bodies, joints = read_pmx(r)
            else:
                raise ValueError('Expected PMD or PMX signature')
        if args.list_bones:
            for bone in bones:
                print('{}\t{}'.format(bone['index'], json.dumps(bone['name'], ensure_ascii=False)))
            return
        selected, included = select_bones(bones, args.bone, args.bone_index)
        chosen = [b for b in bones if b['index'] in included]
        selected_bodies = [b for b in bodies if b['bone'] in included]
        body_ids = {b['index'] for b in selected_bodies}
        chosen_joints = [j for j in joints if j['body_a'] in body_ids or j['body_b'] in body_ids]
        result = {'schema_version': 1, 'format': kind,
                  'coordinates': 'Raw source coordinates; bone positions are model-space bind positions.',
                  'body_position_space': 'bone-local' if kind == 'PMD' else 'model',
                  'body_modes': {'0': 'Kinematic', '1': 'Dynamic', '2': 'DynamicAdjustBone'},
                  'selected_bones': sorted(selected), 'ancestor_bones': sorted(included - selected),
                  'bones': [{k: v for k, v in b.items() if k != 'name' or args.include_names} for b in chosen],
                  'ik': [entry for entry in ik if entry['owner'] in included],
                  'bodies': selected_bodies, 'joints': chosen_joints,
                  'scope': 'Only selected bones and ancestors. References outside this set retain original indices.'}
        if args.motion:
            result['motion'] = {'fps': 30, 'frame_interval': [args.frame_start, args.frame_end],
                                'scope': 'Bone keys only, including nearest bracketing keys; no morph or IK-enable tracks.',
                                'tracks': read_motion(args.motion, chosen, args.frame_start, args.frame_end)}
        payload = json.dumps(result, ensure_ascii=False, indent=2, allow_nan=False) + '\n'
        with args.output.open('x', encoding='utf-8') as output:
            output.write(payload)
        print('Wrote {} bones, {} bodies, {} joints. Review JSON before sharing.'.format(
            len(chosen), len(selected_bodies), len(chosen_joints)))
    except (OSError, ValueError, struct.error) as error:
        # Do not disclose source paths or undecodable source bytes in error messages.
        parser.exit(1, 'Extraction failed ({}). Check file format, selectors and output availability.\n'.format(
            type(error).__name__))


if __name__ == '__main__':
    main()
