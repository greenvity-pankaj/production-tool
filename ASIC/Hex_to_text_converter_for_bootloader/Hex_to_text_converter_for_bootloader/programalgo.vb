Module programalgo

    Dim rd(0 To 63) As Byte
    Dim wr(0 To 63) As Byte
    Dim sentdatalength As Integer
    Dim z As Byte
    Dim recvdatalength As Integer


    Sub proginit_89s52()
        If MPUSBGetDeviceCount(Form1.VIDPID) <> 1 Then

            MsgBox("Can't find Hardware of Programmer !", MsgBoxStyle.Information)

            Exit Sub
        Else
            MsgBox("Found Hardware of Programmer?", MsgBoxStyle.Critical)


            OpenMPUSBDevice()

        End If
        If (MPUSBWrite(myOutPipe, wr(0), 64, SentDataLength, 1000)) Then
            z = MPUSBRead(myInPipe, rd(0), 64, RecvDatalength, 1000)
        Else
            CheckInvalidHandle()

            CloseMPUSBDevice()
            comm_error = True
            ' lbl_status.Text = "Status: Disconnected"
        End If
        CloseMPUSBDevice()
    End Sub









End Module
