Imports System, System.Text, System.Net, System.Net.Sockets

Public Class ConnectedClient

    Const BYTES_TO_READ As Integer = 4096 '2048
    Public mClient As System.Net.Sockets.TcpClient
    Public mDevType As UInteger
    Private RunThread = New Boolean
    Private readThread As System.Threading.Thread
    Private Const MESSAGE_DELIMITER As Char = ControlChars.Cr

    Private lockObj = New Object()

    Public Event dataReceived(ByVal sender As ConnectedClient, ByVal message As Byte())
    '
    '   Sub
    '
    Sub New(ByVal client As System.Net.Sockets.TcpClient)

        mClient = client
        RunThread = True

        readThread = New System.Threading.Thread(AddressOf doRead)
        readThread.IsBackground = True
        readThread.Start()
    End Sub
    '
    '   Clear connection and terminate thread
    '
    Private Sub clearConnection()
        Try
            mClient.GetStream.Flush()
            mClient.GetStream.Close()
            mClient.Close()
        Catch e As Exception
            MsgBox("Unable to close connection. Please restart boards & Windows Tool")
        End Try
        'readThread.Abort()     'Thread abort throws expection, better to do it using a control boolean variable
        RunThread = False
    End Sub
    '
    '   Read data
    '
    Private Sub doRead()

        Dim readBuffer As Byte() = New Byte(BYTES_TO_READ) {}
        Dim bytesRead = New Integer

        Do
            Try
                If mClient.Connected Then
                    If mClient.GetStream.CanRead Then


                        Const buffSize As Integer = 512
                        Dim buffer As Byte() = New Byte(buffSize) {}
                        Dim ret As New UInteger
                        Try
                            Do

                                ret = mClient.GetStream.Read(buffer, 0, buffSize)


                                'If mClient.Connected Then
                                'Try
                                '    With mClient.GetStream
                                '        ret = .Read(buffer, 0, buffSize)
                                '    End With
                                ' Catch ex As SocketException
                                '     If ex.NativeErrorCode.Equals(10053) Then
                                '         clearConnection()
                                '        Array.Clear(readBuffer, 0, BYTES_TO_READ)
                                '       bytesRead = 0
                                '       MsgBox("10053")
                                '       Exit Do
                                '   Else
                                '       MsgBox(ex.ToString)
                                '       Exit Do
                                '    End If
                                '   End Try
                                '  End If
                                Array.Copy(buffer, 0, readBuffer, bytesRead, ret)
                                bytesRead += ret
                                ret = Nothing
                                Array.Clear(buffer, 0, buffSize)
                            Loop While mClient.GetStream.DataAvailable
                        Catch e As Exception
                            bytesRead = 0
                            MsgBox("Communication interrupted. Please restart boards and Windows tool")
                        End Try
                        'bytesRead = mClient.GetStream.Read(readBuffer, 0, BYTES_TO_READ)

                        If (bytesRead > 0) Then
                            SyncLock lockObj
                                RaiseEvent dataReceived(Me, readBuffer)
                            End SyncLock
                            mClient.GetStream.Flush()
                        End If

                        If bytesRead = 0 Then
                            clearConnection()
                        End If

                        Array.Clear(readBuffer, 0, BYTES_TO_READ)
                        bytesRead = 0

                    Else    ' if networkstream is not readable
                        clearConnection()
                    End If
                End If

            Catch ex As SocketException

                If ex.NativeErrorCode.Equals(10053) Then
                    clearConnection()
                    Array.Clear(readBuffer, 0, BYTES_TO_READ)
                    bytesRead = 0
                End If

            End Try

            Threading.Thread.Sleep(5)
        Loop While RunThread

    End Sub
    '
    '   Send data to the client
    '
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
