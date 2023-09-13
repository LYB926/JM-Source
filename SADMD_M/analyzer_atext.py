from file_read_backwards import FileReadBackwards
import matplotlib.pyplot as plt
#rawName = input('Raw video name:   ')
rawName  = 'apple_tree_1920x1080_30'
#para     = input('Parameter (e.g. EPZS&16x16&SR=16&RF=2): ')
QPList   = ['40', '38', '36', '34', '32', '30', '28', '26']
#QPList   = ['41', '38', '35', '32', '28', '26']['26', '28', '30', '32', '34', '36', '38', '40'] 
BRList   = []
PSNRList = []
SSIMList = []
for qp in QPList:
    logName = 'compare/' + rawName + "_" + qp + ".log"
    cnt = 0
    with FileReadBackwards(logName, encoding="utf-8") as BigFile:
    # getting lines by lines starting from the last line up
        for line in BigFile:
            if (cnt==11):
                lineSSIM = line
            if (cnt==14):  #PSNR & SSIM
            #if (cnt==11):  #PSNR ONLY
                linePSNR = line
            if (cnt==6):
                lineBitrate = line
            cnt = cnt + 1
    BRList.append((float(lineBitrate[37:])) /1000)
    PSNRList.append(float(linePSNR[40:-22]))
    SSIMList.append(float(lineSSIM[37:]))
#print (BRList,SSIMList,PSNRList)


x264 = [24.08, 26.72, 30.851629, 32.576346, 33.686060, 35.41703, 36.49]
baseline = [27.00, 28.349293, 32.20 ,33.87, 35.0, 36.67, 37.6]
br264 = [0.7, 1, 2, 3, 4, 6, 8]
# jm   = [29.468,31.420,33.309,35.134,37.396,38.408]
# brjm= [0.936, 1.281, 1.806,2.679,5.150,7.497]
# plt.plot(br264,x264, label='x264', marker='.')
#plt.plot(br264,baseline, label='Target RD curve',linestyle='dashed')
#plt.plot(brjm,jm, label='JM Encoder (Original)', marker='^')

#
#plt.plot(brSub,Sub, label='Disable Sub-partition', marker='h')
#plt.plot(br8x8,d8x8, label='Disable 8x8', marker='p')
#plt.plot(br16,d16, label='Only 16x16', marker='*')
#
jmstd = [29.065, 31.018, 32.899, 34.729, 37.08, 38.11, 39.158, 40.27]
brjmstd = [1.21238, 1.6531, 2.32216, 3.44306, 6.48828, 9.27486, 13.62003, 20.364939999999997]
# plt.plot(brjmstd, jmstd, label='JM Encoder (EPZS&16x16&SR=16&RF=2)', marker='*')
plt.plot(brjmstd, jmstd, label='EPZS&16x16&SR=16&RF=2', linestyle='dotted')#, marker='*')
br03 = [1.86714, 2.219, 2.7079400000000002, 3.36291, 4.3247, 5.76027, 7.964989999999999, 11.34226]
psnr03 = [31.746, 32.878, 33.953, 34.984, 36.03, 37.011, 37.921, 38.828]
br04 = [1.6469200000000002, 1.95831, 2.37829, 2.95795, 3.78668, 5.01996, 6.96333, 10.09003]
psnr04 = [31.266, 32.398, 33.494, 34.51, 35.599, 36.626, 37.602, 38.592]
br05 = [1.50815, 1.80771, 2.17862, 2.6911199999999997, 3.38682, 4.40967, 6.02289, 8.88166]
psnr05 = [30.833, 31.959, 33.047, 34.049, 35.112, 36.105, 37.113, 38.208]

br06 = [1.4113900000000001, 1.6910399999999999, 2.05897, 2.49729, 3.14292, 3.99102, 5.35242, 7.73556]
psnr06 = [30.378, 31.526, 32.601, 33.625, 34.659, 35.633, 36.61, 37.685]
br07 = [1.3378599999999998, 1.58954, 1.9461700000000002, 2.3324000000000003, 2.93498, 3.73307, 4.9333599999999995, 6.9322799999999996]
psnr07 = [29.969, 31.09, 32.169, 33.23, 34.263, 35.228, 36.184, 37.217]
br08 = [1.28102, 1.53603, 1.84163, 2.24458, 2.82022, 3.56305, 4.69245, 6.58188]
psnr08 = [29.594, 30.71, 31.787, 32.839, 33.899, 34.872, 35.892, 36.942]
br09 = [1.23386, 1.4779200000000001, 1.79067, 2.15774, 2.70347, 3.44075, 4.5335, 6.35603]
psnr09 = [29.284, 30.386, 31.429, 32.508, 33.583, 34.621, 35.656, 36.754]
br10 = [1.18468, 1.43167, 1.72558, 2.10166, 2.62156, 3.34341, 4.4449499999999995, 6.194229999999999]
psnr10 = [28.997, 30.091, 31.136, 32.206, 33.292, 34.368, 35.461, 36.578]
br13 = [1.10829, 1.33601, 1.61882, 1.9841900000000001, 2.5046399999999998, 3.21439, 4.27691, 5.95209]
psnr13 = [28.311, 29.384, 30.434, 31.522, 32.667, 33.844, 35.017, 36.302]

br15 = [1.08853, 1.2964, 1.5821800000000001, 1.93791, 2.46164, 3.14066, 4.1949, 5.90447]
psnr15 = [27.956, 29.046, 30.108, 31.214, 32.368, 33.583, 34.804, 36.016]
br18 = [1.03632, 1.26431, 1.53561, 1.8968399999999999, 2.39557, 3.0870100000000003, 4.134930000000001, 5.82275]
psnr18 = [27.552, 28.643, 29.71, 30.872, 32.065, 33.258, 34.523, 35.776]
#labelJM = 'JM Encoder (' + para + ')'
#plt.plot(BRList, PSNRList, label='$\\alpha = 0.6$', marker='o')
#plt.plot(br04, psnr04, label='$\\alpha = 0.4$', marker='.')
#plt.plot(br05, psnr05, label='$\\alpha = 0.5$', marker='<')

plt.plot(br06, psnr06, label='$\\alpha = 0.6$', marker='*')
plt.plot(br07, psnr07, label='$\\alpha = 0.7$', marker='.')
plt.plot(br08, psnr08, label='$\\alpha = 0.8$', marker='<')
plt.plot(br09, psnr09, label='$\\alpha = 0.9$', marker='o')
plt.plot(br10, psnr10, label='$\\alpha = 1.0$', marker='v')


#plt.plot(br15, psnr15, label='$\\alpha = 1.5$', marker='D')
#plt.plot(br18, psnr18, label='$\\alpha = 1.8$', marker='^')
#plt.plot(br12, psnr12, label='1.2', marker='s')

#plt.plot(br14, psnr14, label='1.4', marker='x')
#plt.plot(br15, psnr15, label='$\\alpha = 1.5$', marker='h')
plt.xlabel('Bitrate, Mbps')
plt.ylabel('PSNR(Y), dB') 
plt.title('\"apple_tree\" sequence, Bitrate/quality, PSNR Metric, SAD MD, $\lambda_{MD} = \\alpha \lambda_{ME}$')  
plt.legend()
#plt.savefig("./" + para + '.png')
plt.show()