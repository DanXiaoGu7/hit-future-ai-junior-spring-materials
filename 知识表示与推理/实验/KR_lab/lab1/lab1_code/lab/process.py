import jieba

# 自定义词，确保这些词在分词后不会被拆开
jieba.add_word("蓝银草")

with open('./data/text_new.txt', errors='ignore', encoding='utf-8') as fp:
   lines = fp.readlines()
   for line in lines:
       seg_list = jieba.cut(line)
       with open('./data/seg_text.txt', 'a', encoding='utf-8') as ff:
           ff.write(' '.join(seg_list))
