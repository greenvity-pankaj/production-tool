Imports System.Text, System.IO, System.Xml
Imports System.Globalization

Public Class readConfig

    Public path As String = System.IO.Path.Combine(HomeScreen.rootFilePath, "settings.xml")

    Public Sub readXML()

        Try
            Using xmlString As StreamReader = New StreamReader(path)

                Using reader As XmlReader = XmlReader.Create(New StringReader(xmlString.ReadToEnd.ToString))
                    While reader.Read

                        If reader.Name = "threshold" Then
                            If reader.Read Then
                                'MessageBox.Show(reader.Value.ToString)
                                HomeScreen.threshold = reader.ReadContentAsInt
                                reader.Read()
                            End If
                        End If

                        If reader.Name = "channel" Then
                            If reader.Read Then
                                'MessageBox.Show(reader.Value.ToString)
                                reader.Read()
                            End If
                        End If

                        If reader.Name = "FrameLen" Then
                            If reader.Read Then
                                'MessageBox.Show(reader.Value.ToString)
                                reader.Read()
                            End If
                        End If

                        ' Read MAC Address start to 
                        If reader.Name = "startMacAddr" Then
                            If reader.Read Then
                                Dim str As String = String.Join("", reader.Value.ToString.Split(":"))
                                HomeScreen.gMACcounter = ULong.Parse(str, NumberStyles.HexNumber, CultureInfo.CurrentCulture.NumberFormat)
                                Dim t As String = Hex(HomeScreen.gMACcounter)
                                Dim i As Integer = 2
                                Dim s As StringBuilder = New StringBuilder(t)
                                While True
                                    s.Insert(i, ":")
                                    i += 3
                                    If i >= s.Length Then
                                        Exit While
                                    End If
                                End While
                                reader.Read()
                            End If
                        End If

                    End While
                End Using
            End Using
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try

    End Sub

End Class

