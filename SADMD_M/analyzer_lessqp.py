from file_read_backwards import FileReadBackwards
import matplotlib.pyplot as plt
#rawName = input('Raw video name:   ')
rawName  = 'mordor_gameplay_1920x1080_39'
#para     = input('Parameter (e.g. EPZS&16x16&SR=16&RF=2): ')
#QPList   = ['40', '38', '36', '34', '32', '30', '28', '26']
QPList   = ['40', '36', '32', '28']
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
jmstd = [28.107, 29.035, 29.997, 31.105, 32.472, 34.073, 35.835, 37.736]
brjmstd = [3.49404, 5.261609999999999, 7.65412, 11.2188, 17.02742, 25.58959, 36.53706, 49.62522]
# plt.plot(brjmstd, jmstd, label='JM Encoder (EPZS&16x16&SR=16&RF=2)', marker='*')
plt.plot(brjmstd, jmstd, label='EPZS&16x16&SR=16&RF=2', linestyle='dotted')#, marker='*')
br04 = [6.21581, 11.60014, 22.949060000000003, 45.51752]
psnr04 = [26.584, 28.03, 30.219, 33.528]
br05 = [5.40415, 10.25199, 20.89116, 42.39932]
psnr05 = [26.352, 27.75, 29.895, 33.113]

#labelJM = 'JM Encoder (' + para + ')'
#plt.plot(BRList, PSNRList, label='$\\alpha = 0.6$', marker='o')

plt.plot(br04, psnr04, label='$\\alpha = 0.4$', marker='<')
plt.plot(br05, psnr05, label='$\\alpha = 0.5$', marker='*')
plt.plot(BRList, PSNRList, label='$\\alpha = 0.6$', marker='o')
#plt.plot(br07, psnr07, label='$\\alpha = 0.7$', marker='.')
#plt.plot(br10, psnr10, label='$\\alpha = 1.0$', marker='v')
#plt.plot(br07, psnr07, label='$\\alpha = 0.7$', marker='D')
#plt.plot(br11, psnr11, label='1.1', marker='^')
#plt.plot(br12, psnr12, label='1.2', marker='s')
#plt.plot(br13, psnr13, label='$\\alpha = 1.3$', marker='o')
#plt.plot(br14, psnr14, label='1.4', marker='x')
#plt.plot(br15, psnr15, label='$\\alpha = 1.5$', marker='h')
plt.xlabel('Bitrate, Mbps')
plt.ylabel('PSNR(Y), dB') 
plt.title('\"mordor_gameplay\" sequence, Bitrate/quality, PSNR Metric, SAD MD, $\lambda_{MD} = \\alpha \lambda_{ME}$')  
plt.legend()
#plt.savefig("./" + para + '.png')
plt.show()