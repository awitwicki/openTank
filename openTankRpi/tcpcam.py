import cv2

# raspivid -n -ih -t 0 -rot 0 -w 1280 -h 720 -fps 15 -b 1000000 -o - | nc -lkv4 5001

rpi_ip = 'tcp://192.168.1.3:5001'
cap = cv2.VideoCapture(rpi_ip, cv2.CAP_FFMPEG)

while True:
    ret, frame = cap.read()

    if not ret:
        print('frame empty')
    else:
        cv2.imshow('image', cv2.resize(frame, (640, 360)))

    if cv2.waitKey(1)&0XFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
