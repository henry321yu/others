import cv2

# Start the camera (default camera index 0)
cap = cv2.VideoCapture(0)

if not cap.isOpened():
    print("Unable to start the camera")
    exit()

while True:
    # Capture frame from the camera
    ret, frame = cap.read()

    if not ret:
        print("Unable to retrieve frame")
        break

    # Display the frame
    cv2.imshow("Webcam", frame)

    # Press 'ESC' key to exit
    if cv2.waitKey(1) & 0xFF == 27:
        break

# Release the camera and close the window
cap.release()
cv2.destroyAllWindows()
