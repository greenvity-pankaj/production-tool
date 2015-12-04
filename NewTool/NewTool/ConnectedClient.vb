Imports System, System.Text, System.Net, System.Net.Sockets

Public Class ConnectedClient

    Const BYTES_TO_READ As Integer = 256
    Public mClient As System.Net.Sockets.TcpClient
    Public mDevType As UInteger
    Private readThread As System.Threading.Thread
    Private Const MESSAGE_DELIMITER As Char = ControlChars.Cr

    Private lockObj = New Object()

    Public Event dataReceived(ByVal sender As ConnectedClient, ByVal message As Byte())
    '
    '   Sub
    '
    Sub New(ByVal client As System.Net.Sockets.TcpClient)

        mClient = client

        readThread = New System.Threading.Thread(AddressOf doRead)
        readThread.IsBackground = True
        readThread.Start()
        HomeScreen.dumpMsg("")
    End Sub
    '
    '   Read data
    '
    Private Sub doRead()

        Dim readBuffer As Byte() = New Byte(BYTES_TO_READ) {}
        Dim bytesRead As Integer

        Try
            Do
                If mClient.GetStream.CanRead Then
                    bytesRead = mClient.GetStream.Read(readBuffer, 0, BYTES_TO_READ)
                    If (bytesRead > 0) Then

                        'The first element in the subMessages string array must be the last part of the current message.
                        'So we append it to the StringBuilder and raise the dataReceived event
                        'Dim lastByte As Byte = readBuffer(bytesRead)
                        'If ControlChars.Cr.CompareTo(Convert.ToChar(lastByte)) Then
                        'Dim packet(bytesRead) As Byte
                        'Array.Copy(readBuffer, packet, bytesRead)

                        SyncLock lockObj
                            RaiseEvent dataReceived(Me, readBuffer)
                        End SyncLock

                        mClient.GetStream.Flush()
                        Array.Clear(readBuffer, 0, BYTES_TO_READ)
                        bytesRead = 0

                    End If

                    'If bytesRead = 0 Then
                    '    mClient.Close()
                    'End If
                End If
            Loop
        Catch ex As SocketException
            MessageBox.Show(ex.ToString)
        End Try

    End Sub

    Public Sub SendMessage(ByVal msg As Byte())

        Try
            SyncLock lockObj
                If mClient.GetStream.CanWrite Then
                    mClient.GetStream.Write(msg, 0, msg.Length)
                End If
            End SyncLock
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try
    End Sub
End Class
