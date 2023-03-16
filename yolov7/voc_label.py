import xml.etree.ElementTree as ET
import os
from os import getcwd

sets = ['train', 'val', 'test']
classes = [ 'red_ju', 'red_bing', 'red_xiang', 'black_zu', 'red_pao', 'red_ma',
                   'black_ma', 'red_shi', 'red_shuai', 'black_pao', 'black_xiang',
                   'black_shi', 'black_jiang', 'black_ju' ]  # 写自己类别
abs_path = os.getcwd()
print(abs_path)


def convert(size, box):
    dw = 1. / (size[0])
    dh = 1. / (size[1])
    x = (box[0] + box[1]) / 2.0 - 1
    y = (box[2] + box[3]) / 2.0 - 1
    w = box[1] - box[0]
    h = box[3] - box[2]
    x = x * dw
    w = w * dw
    y = y * dh
    h = h * dh
    return x, y, w, h


def convert_annotation(image_id):
    in_file = open('/home/ubuntu/zfk/dataset/zong/xml/%s.xml' % (image_id), encoding='UTF-8')
    out_file = open('/home/ubuntu/zfk/dataset/zong/labels/%s.txt' % (image_id), 'w')
    tree = ET.parse(in_file)
    root = tree.getroot()
    size = root.find('size')
    w = int(size.find('width').text)
    h = int(size.find('height').text)
    print(in_file)
    for obj in root.iter('object'):
        difficult = obj.find('difficult').text
        cls = obj.find('name').text
        if cls not in classes or int(difficult) == 1:
            continue
        cls_id = classes.index(cls)
        xmlbox = obj.find('bndbox')
        b = (float(xmlbox.find('xmin').text), float(xmlbox.find('xmax').text), float(xmlbox.find('ymin').text),
             float(xmlbox.find('ymax').text))

        b1, b2, b3, b4 = b
        if b2 > w:
            b2 = w
        if b4 > h:
            b4 = h
        b = (b1, b2, b3, b4)
        bb = convert((w, h), b)
        out_file.write(str(cls_id) + " " + " ".join([str(a) for a in bb]) + '\n')


wd = getcwd()
# for image_set in sets:
#     if not os.path.exists('F:/chess_data/labels/'):
#         os.makedirs('F:/chess_data/labels/')
#     image_ids = open('F:/chess_data/imageset/%s.txt' % (image_set)).read().strip().split()
#     list_file = open('F:/chess_data/%s.txt' % (image_set), 'w')
#     for image_id in image_ids:
#         #list_file.write(abs_path + 'F:/chess_data/images/%s.jpg\n' % (image_id))
#         convert_annotation(image_id)
#     list_file.close()
xmlfilepath = '/home/ubuntu/zfk/dataset/zong/xml'
total_xml = os.listdir(xmlfilepath)
num = len(total_xml)
list = range(num)
print(total_xml)
for i in list:
    name = total_xml[i][:-4]
    convert_annotation(name)