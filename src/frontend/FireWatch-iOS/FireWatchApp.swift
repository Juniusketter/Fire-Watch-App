import SwiftUI
import WebKit

struct FireWatchAppView: View {
    @State private var address = "https://firewatch.example"
    @State private var currentURL: URL? = URL(string: "https://firewatch.example")

    var body: some View {
        NavigationView {
            VStack(spacing: 12) {
                HStack(spacing: 8) {
                    TextField("Enter FireWatch URL", text: $address)
                        .textInputAutocapitalization(.never)
                        .keyboardType(.URL)
                        .textFieldStyle(RoundedBorderTextFieldStyle())

                    Button("Open") {
                        if let url = URL(string: address) {
                            currentURL = url
                        }
                    }
                }

                if let currentURL {
                    WebView(url: currentURL)
                } else {
                    Text("Enter a valid URL to open FireWatch.")
                        .foregroundColor(.secondary)
                        .frame(maxWidth: .infinity, maxHeight: .infinity)
                }
            }
            .padding()
            .navigationTitle("FireWatch")
        }
    }
}

struct WebView: UIViewRepresentable {
    let url: URL

    func makeUIView(context: Context) -> WKWebView {
        let webView = WKWebView()
        webView.load(URLRequest(url: url))
        return webView
    }

    func updateUIView(_ webView: WKWebView, context: Context) {
        if webView.url != url {
            webView.load(URLRequest(url: url))
        }
    }
}

@main
struct FireWatchApp: App {
    var body: some Scene {
        WindowGroup {
            FireWatchAppView()
        }
    }
}
