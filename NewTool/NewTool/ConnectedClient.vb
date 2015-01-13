Imports System, System.Text, System.Net, System.Net.Sockets

Public Class ConnectedClient

    Const BYTES_TO_READ As Integer = 1024
    Public mClient As System.Net.Sockets.TcpClient
    Public mDevType As UInteger
    Private readThread As System.Threading.Thread
    Private Const MESSAGE_DELIMITER As Char = ControlChars.Cr

    Public Event dataReceived(ByVal sender As ConnectedClient, ByVal message As Byte())
    '
    '   Sub
    '
    Sub New(ByVal client As System.Net.Sockets.TcpClient)

        mClient = client

        readThread = New System.Threading.Thread(AddressOf doRead)
        readThread.IsBackground = True
        readThread.Start()
    End Sub
    '
    '   Read data
    '
    Private Sub doRead()

        Dim readBuffer As Byte() = New Byte(BYTES_TO_READ) {}
        Dim bytesRead As Integer

        Try
            Do
                bytesRead = mClient.GetStream.Read(readBuffer, 0, BYTES_TO_READ)
                If (bytesRead > 0) Then

                    'The first element in the subMessages string array must be the last part of the current message.
                    'So we append it to the StringBuilder and raise the dataReceived event
                    Dim lastByte As Byte = readBuffer(bytesRead)
                    If ControlChars.Cr.CompareTo(Convert.ToChar(lastByte)) Then
                        Dim packet(bytesRead) As Byte
                        Array.Copy(readBuffer, packet, bytesRead)
                        RaiseEvent dataReceived(Me, packet)
                    End If
                End If
            Loop
        Catch ex As Exception
            Throw ex
        End Try

    End Sub

    Public Sub SendMessage(ByVal msg As Byte())

        Try
            SyncLock mClient.GetStream
                If mClient.GetStream.CanWrite Then
                    mClient.GetStream.Write(msg, 0, msg.Length)
                End If
            End SyncLock
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try
    End Sub
End Class
